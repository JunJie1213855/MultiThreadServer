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
    std::cout << "~CSession destruct" << endl;
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

void CSession::StartCoroutine()
{
    co_spawn(_socket.get_executor(), [self = shared_from_this()]
             { return self->HandleWrite(); }, detached);
}

void CSession::Send(std::string msg, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size >= MAX_SENDQUE)
    {
        std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    _send_que.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
    if (send_que_size == 0)
    {
        co_spawn(_socket.get_executor(), [self = shared_from_this()]
                 { return self->HandleWrite(); }, detached);
    }
}

void CSession::Send(char *msg, short max_length, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    _send_que.push(make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size == 0)
    {
        co_spawn(_socket.get_executor(), [self = shared_from_this()]
                 { return self->HandleWrite(); }, detached);
    }
}

void CSession::Close()
{
    _socket.close();
    _b_close = true;
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
    return shared_from_this();
}

awaitable<void> CSession::HandleWrite()
{
    try
    {
        while (!_b_close)
        {
            shared_ptr<SendNode> msgnode;

            // 获取消息
            {
                std::lock_guard<std::mutex> lock(_send_lock);
                if (_send_que.empty())
                {
                    co_return;
                }
                msgnode = std::move(_send_que.front());
                _send_que.pop();
            }

            co_await async_write(_socket,
                                 buffer(msgnode->_data, msgnode->_total_len),
                                 use_awaitable);
        }
    }
    catch (boost::system::error_code &ec)
    {
        std::cerr << "Handle Write Exception code: " << ec.what() << endl;
    }
}

awaitable<void> CSession::HandleRead()
{
    try
    {
        while (!_b_close)
        {
            short msg_id = 0;
            short msg_len = 0;

            // Read header
            _recv_head_node->Clear();
            std::size_t n = co_await async_read(_socket,
                                                buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
                                                use_awaitable);

            if (n != HEAD_TOTAL_LEN)
            {
                break;
            }

            // Parse msg_id
            memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
            std::cout << "msg_id is " << msg_id << endl;

            if (msg_id > MAX_LENGTH)
            {
                std::cout << "invalid msg_id is " << msg_id << endl;
                _server->ClearSession(_uuid);
                co_return;
            }

            // Parse msg_len
            memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
            std::cout << "msg_len is " << msg_len << endl;

            if (msg_len > MAX_LENGTH)
            {
                std::cout << "invalid data length is " << msg_len << endl;
                _server->ClearSession(_uuid);
                co_return;
            }

            _recv_msg_node = make_shared<RecvNode>(msg_len, msg_id);

            // Read msg body
            n = co_await async_read(_socket,
                                    buffer(_recv_msg_node->_data, msg_len),
                                    use_awaitable);

            if (n != static_cast<std::size_t>(msg_len))
            {
                break;
            }

            _recv_msg_node->_cur_len = msg_len;
            _recv_msg_node->_data[msg_len] = '\0';

            LogicSystem::GetInstance()->PostMsgToQue(
                make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
        }
    }
    catch (boost::system::error_code &ec)
    {
        std::cout << "Handle Read Exception code is " << ec.what() << endl;
        Close();
        _server->ClearSession(_uuid);
    }
}

LogicNode::LogicNode(shared_ptr<CSession> session,
                     shared_ptr<RecvNode> recvnode) : _session(session), _recvnode(recvnode)
{
}