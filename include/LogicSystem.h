#ifndef LOGICSYSTEM_H_
#define LOGICSYSTEM_H_

#include "Singleton.h"
#include <queue>
#include <thread>
#include "CSession.h"
#include <queue>
#include <map>
#include <functional>
#include "const.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <atomic>
#include <chrono>

typedef function<void(shared_ptr<CSession>, short msg_id, string msg_data)> FunCallBack;
class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;

public:
	~LogicSystem();
	void PostMsgToQue(shared_ptr<LogicNode> msg);

	void RecordRequest();
	void RecordResponse();
	int64_t GetTotalRequests();
	int64_t GetTotalResponses();
	double GetQPS();
	double GetResponseRate();
	void PrintStatistics();
	void ResetStatistics();

private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void HelloWordCallBack(shared_ptr<CSession>, short msg_id, string msg_data);

private:
	std::thread _worker_thread;
	std::queue<shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks;

	std::atomic<int64_t> _total_requests;
	std::atomic<int64_t> _total_responses;
	std::atomic<int64_t> _last_requests;
	std::chrono::steady_clock::time_point _start_time;
	std::chrono::steady_clock::time_point _last_print_time;
};

#endif