#ifndef LOGICSYSTEM_H_
#define LOGICSYSTEM_H_

#include <queue>
#include <thread>
#include <map>
#include <functional>
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
	void RegisterCallBacks();
	void HelloWordCallBack(std::shared_ptr<CSession> session, short msg_id, const std::string &msg_data);

private:
	std::map<short, FunCallBack> _fun_callbacks; // 存储 msgid 对应的处理方法函数
};

#endif