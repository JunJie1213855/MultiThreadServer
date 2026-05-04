#include "LogicSystem.h"

using namespace std;

LogicSystem::LogicSystem() : _b_stop(false)
{
	RegisterCallBacks();
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}

// 添加消息到处理队列
void LogicSystem::PostMsgToQue(shared_ptr<LogicNode> msg)
{
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);
	if (_msg_que.size() == 1)
	{
		unique_lk.unlock();
		_consume.notify_one();
	}
}

// 消息处理线程函数
void LogicSystem::DealMsg()
{
	for (;;)
	{
		std::unique_lock<std::mutex> unique_lk(_mutex);
		// 等待条件变量唤醒
		while (_msg_que.empty() && !_b_stop)
		{
			_consume.wait(unique_lk);
		}

		// 如果停止标志为 true，处理完剩余消息后退出
		if (_b_stop)
		{
			while (!_msg_que.empty())
			{
				auto msg_node = _msg_que.front();
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == _fun_callbacks.end())
				{
					_msg_que.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
									   std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop();
			}
			break;
		}

		// 取出队首消息并处理
		auto msg_node = _msg_que.front();
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end())
		{
			_msg_que.pop();
			continue;
		}
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
							   std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop();
	}
}

// 注册消息回调函数
void LogicSystem::RegisterCallBacks()
{
	_fun_callbacks[MSG_HELLO_WORD] = std::bind(&LogicSystem::HelloWordCallBack, this,
												placeholders::_1, placeholders::_2, placeholders::_3);
}

// 处理 HELLO_WORD 消息的回调
void LogicSystem::HelloWordCallBack(shared_ptr<CSession> session, short msg_id, string msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	std::cout << "recevie msg id  is " << root["id"].asInt() << " msg data is "
			  << root["data"].asString() << endl;
	root["data"] = "server has received msg, msg data is " + root["data"].asString();
	std::string return_str = root.toStyledString();
	session->Send(return_str, root["id"].asInt());
}