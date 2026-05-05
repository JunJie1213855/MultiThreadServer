#ifndef LOGICSYSTEM_H_
#define LOGICSYSTEM_H_

#include <queue>
#include <thread>
#include <queue>
#include <map>
#include <functional>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CSession.h"
#include "const.h"
#include "Singleton.h"

class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
	using FunCallBack = std::function<void(std::shared_ptr<CSession>, short msg_id, std::string msg_data)>;

public:
	~LogicSystem();
	void PostMsgToQue(std::shared_ptr<LogicNode> msg);

private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void HelloWordCallBack(std::shared_ptr<CSession>, short msg_id, std::string msg_data);

private:
	std::thread _worker_thread;
	std::queue<std::shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks;
};

#endif