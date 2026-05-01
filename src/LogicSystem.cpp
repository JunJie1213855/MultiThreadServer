#include "LogicSystem.h"

using namespace std;

LogicSystem::LogicSystem() : _b_stop(false),
							 _total_requests(0),
							 _total_responses(0),
							 _last_requests(0)
{
	_start_time = chrono::steady_clock::now();
	_last_print_time = _start_time;
	RegisterCallBacks();
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
	PrintStatistics();
}

// 记录收到请求
void LogicSystem::RecordRequest()
{
	_total_requests++;
}

// 记录发送响应
void LogicSystem::RecordResponse()
{
	_total_responses++;
}

// 获取总请求数
int64_t LogicSystem::GetTotalRequests()
{
	return _total_requests.load();
}

// 获取总响应数
int64_t LogicSystem::GetTotalResponses()
{
	return _total_responses.load();
}

// 计算当前 QPS（每秒处理请求数）
double LogicSystem::GetQPS()
{
	auto now = chrono::steady_clock::now();
	auto duration = chrono::duration_cast<chrono::seconds>(now - _last_print_time).count();

	if (duration <= 0)
		return 0;

	// 计算增量请求数
	int64_t current_requests = _total_requests.load() - _last_requests.load();
	_last_requests.store(_total_requests.load());
	_last_print_time = now;

	return (double)current_requests / duration;
}

// 计算回包完整率
double LogicSystem::GetResponseRate()
{
	int64_t req = _total_requests.load();
	int64_t resp = _total_responses.load();

	if (req == 0)
		return 0;

	return (double)resp / req * 100;
}

// 打印统计信息
void LogicSystem::PrintStatistics()
{
	auto now = chrono::steady_clock::now();
	auto total_duration = chrono::duration_cast<chrono::seconds>(now - _start_time).count();

	double avg_qps = (double)_total_requests.load() / (total_duration > 0 ? total_duration : 1);

	std::cout << "========== Server Statistics ==========" << std::endl;
	std::cout << "Total Requests:   " << _total_requests.load() << std::endl;
	std::cout << "Total Responses:  " << _total_responses.load() << std::endl;
	std::cout << "Response Rate:    " << GetResponseRate() << "%" << std::endl;
	std::cout << "Average QPS:      " << avg_qps << std::endl;
	std::cout << "Uptime:           " << total_duration << "s" << std::endl;
	std::cout << "=========================================" << std::endl;
}

// 重置统计
void LogicSystem::ResetStatistics()
{
	_total_requests = 0;
	_total_responses = 0;
	_last_requests = 0;
	_start_time = chrono::steady_clock::now();
	_last_print_time = _start_time;
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
				cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
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
		cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
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
	RecordRequest();

	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	std::cout << "recevie msg id  is " << root["id"].asInt() << " msg data is "
			  << root["data"].asString() << endl;
	root["data"] = "server has received msg, msg data is " + root["data"].asString();
	std::string return_str = root.toStyledString();
	session->Send(return_str, root["id"].asInt());

	RecordResponse();
}