#include "CSession.h"
#include "CServer.h"
#include <iostream>
#include <sstream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "LogicSystem.h"
#include <boost/asio/use_awaitable.hpp>

using namespace std;
using namespace boost::asio;

CSession::CSession(boost::asio::io_context &io_context, CServer *server)
    : _socket(io_context), _server(server), _b_close(false), _b_head_parse(false)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession()
{
}

tcp::socket &CSession::GetSocket()
{
    return _socket;
}

std::string &CSession::GetUuid()
{
    return _uuid;
}

void CSession::Start()
{
    co_spawn(_socket.get_executor(), [self = shared_from_this()]
             { return self->HandleRead(); }, detached);
}

void CSession::Send(std::string msg, short msgid)
{
    boost::asio::post(_socket.get_executor(), [self = shared_from_this(), msg = std::move(msg), msgid]() mutable
                      {
    int send_que_size = self->_send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << self->_uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    self->_send_que.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
    if (send_que_size == 0) // 如果队列深度数量为 0，说明没有handlewrite协程了
    {
        co_spawn(self->_socket.get_executor(), [self]
                 { return self->HandleWrite(); }, detached);
    } });
}

void CSession::Send(char *msg, short max_length, short msgid)
{
    boost::asio::post(_socket.get_executor(), [self = shared_from_this(), msg = std::move(msg), max_length, msgid]() mutable
                      {
    int send_que_size = self->_send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << self->_uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    self->_send_que.push(make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size == 0) // 如果队列深度数量为 0，说明没有handlewrite协程了
    {
        co_spawn(self->_socket.get_executor(), [self]
                 { return self->HandleWrite(); }, detached);
    } });
}

void CSession::Close()
{
    boost::asio::post(_socket.get_executor(), [self = shared_from_this()]
                      {
        if(self->_b_close) return;
        self->_b_close = true;
        boost::system::error_code ec;
        self->_socket.close(ec); 
        self->_server->ClearSession(self->GetUuid()); });
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
    return shared_from_this();
}

awaitable<void> CSession::HandleWrite()
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
                // _server->ClearSession(_uuid);
                co_return;
            }
        }

        if (success)
            _send_que.pop();
    }
}

awaitable<void> CSession::HandleRead()
{
    while (!_b_close)
    {
        short msg_id = 0;
        short msg_len = 0;

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

        // Parse msg_id
        memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
        msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);

        if (msg_id <= MSG_ID_MIN || msg_id >= MSG_ID_MAX)
        {
            std::cout << "invalid msg_id is " << msg_id << endl;
            _server->ClearSession(_uuid);
            co_return;
        }

        // Parse msg_len
        memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
        msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

        if (msg_len > MAX_LENGTH)
        {
            std::cout << "invalid data length is " << msg_len << endl;
            _server->ClearSession(_uuid);
            co_return;
        }

        _recv_msg_node = make_shared<RecvNode>(msg_len, msg_id);

        // Read msg body
        {
            auto [ec, n] = co_await async_read(_socket,
                                               buffer(_recv_msg_node->_data, msg_len),
                                               as_tuple(use_awaitable));
            if (ec)
            {
                Close();
                co_return;
            }
            if (n != static_cast<std::size_t>(msg_len))
            {
                break;
            }
        }

        _recv_msg_node->_cur_len = msg_len;
        _recv_msg_node->_data[msg_len] = '\0';

        LogicSystem::GetInstance()->PostMsgToQue(
            make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
    }
}

LogicNode::LogicNode(std::shared_ptr<CSession> session,
                     std::shared_ptr<RecvNode> recvnode) : _session(session), _recvnode(recvnode)
{
}