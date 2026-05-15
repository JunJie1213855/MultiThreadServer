#include "mts/session.h"
#include "mts/server.h"
#include "mts/frame.h"
#include <iostream>
#include <boost/asio/use_awaitable.hpp>

namespace mts
{

	using namespace std;
	using namespace boost::asio;

	TCPSession::TCPSession(boost::asio::io_context &io_context, TCPServer *server)
		: _socket(io_context), _server(server), _b_close(false), _b_head_parse(false)
	{
		boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
		_uuid = boost::uuids::to_string(a_uuid);
		_recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
	}

	TCPSession::~TCPSession()
	{
	}

	tcp::socket &TCPSession::GetSocket()
	{
		return _socket;
	}

	std::string &TCPSession::GetUuid()
	{
		return _uuid;
	}

	void TCPSession::Start()
	{
		co_spawn(_socket.get_executor(), [self = shared_from_this()]
				 { return self->HandleRead(); }, detached);
	}

	void TCPSession::Send(std::string msg, short msgid)
	{
		boost::asio::post(_socket.get_executor(), [self = shared_from_this(), msg = std::move(msg), msgid]() mutable
						  {
    int send_que_size = self->_send_que.size();
    if (static_cast<std::size_t>(send_que_size) > self->_server->config().max_send_queue)
    {
        std::cout << "session: " << self->_uuid << " send que fulled, size is " << self->_server->config().max_send_queue << endl;
        return;
    }

    self->_send_que.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
    if (send_que_size == 0) // 如果队列深度数量为 0，说明没有handlewrite协程了
    {
        co_spawn(self->_socket.get_executor(), [self]
                 { return self->HandleWrite(); }, detached);
    } });
	}

	void TCPSession::Send(char *msg, short max_length, short msgid)
	{
		boost::asio::post(_socket.get_executor(), [self = shared_from_this(), msg = std::move(msg), max_length, msgid]() mutable
						  {
    int send_que_size = self->_send_que.size();
    if (static_cast<std::size_t>(send_que_size) > self->_server->config().max_send_queue)
    {
        std::cout << "session: " << self->_uuid << " send que fulled, size is " << self->_server->config().max_send_queue << endl;
        return;
    }

    self->_send_que.push(make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size == 0) // 如果队列深度数量为 0，说明没有handlewrite协程了
    {
        co_spawn(self->_socket.get_executor(), [self]
                 { return self->HandleWrite(); }, detached);
    } });
	}

	void TCPSession::Close()
	{
		boost::asio::post(_socket.get_executor(), [self = shared_from_this()]
						  {
        if(self->_b_close) return;
        self->_b_close = true;
        boost::system::error_code ec;
        self->_socket.close(ec);
        self->_server->clear_session(self->GetUuid()); });
	}

	std::shared_ptr<TCPSession> TCPSession::SharedSelf()
	{
		return shared_from_this();
	}

	awaitable<void> TCPSession::HandleWrite()
	{
		while (!_b_close) // 不停止且队列不为空
		{
			if (_send_que.empty())
				co_return;

			std::shared_ptr<SendNode> msgnode;
			msgnode = _send_que.front();

			bool success = false;
			for (int retry = 0; retry <= 3 && !_b_close; ++retry)
			{
				if (retry > 0) // 重试三次
				{
					boost::asio::steady_timer t(_socket.get_executor());
					t.expires_after(std::chrono::milliseconds(50 * retry));
					co_await t.async_wait(boost::asio::use_awaitable); // 等待
				}

				auto [ec, n] = co_await async_write(_socket,
													buffer(msgnode->_data, msgnode->_total_len),
													as_tuple(use_awaitable));

				if (!ec)
				{
					success = true;
					break;
				}

				if (retry == 3)
				{
					std::cout << "could not connect the client, close the session" << std::endl;
					Close();
					co_return;
				}
			}

			if (success)
				_send_que.pop();
		}
	}

	awaitable<void> TCPSession::HandleRead()
	{
		while (!_b_close)
		{
			// Read header
			_recv_head_node->Clear();
			{
				auto [ec, n] = co_await async_read(_socket,
												   buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
												   as_tuple(use_awaitable));
				if (ec)
				{
					Close();
					co_return;
				}
				if (n != HEAD_TOTAL_LEN)
				{
					co_return;
				}
			}

			// 解码包头（网络序 -> 主机序）。框架不再校验 msg_id 范围（那是业务语义）：
			// 未注册的 msg_id 在 Dispatcher 中是无害的 no-op。
			FrameHeader header = parse_header(_recv_head_node->_data);

			if (!header_is_valid(header, _server->config().max_msg_length))
			{
				std::cout << "invalid data length is " << header.body_len << endl;
				_server->clear_session(_uuid);
				co_return;
			}

			_recv_msg_node = make_shared<RecvNode>(header.body_len, header.msg_id);

			// Read msg body
			{
				auto [ec, n] = co_await async_read(_socket,
												   buffer(_recv_msg_node->_data, header.body_len),
												   as_tuple(use_awaitable));
				if (ec)
				{
					Close();
					co_return;
				}
				if (n != static_cast<std::size_t>(header.body_len))
				{
					break;
				}
			}

			_recv_msg_node->_cur_len = header.body_len;
			_recv_msg_node->_data[header.body_len] = '\0';

			_server->dispatcher().dispatch(shared_from_this(), _recv_msg_node);
		}
	}

} // namespace mts
