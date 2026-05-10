#include "LogicSystem.h"

using namespace std;

LogicSystem::LogicSystem()
{
	RegisterCallBacks();
}

LogicSystem::~LogicSystem()
{
}

// 把消息送到session对应的io_context处理
void LogicSystem::PostMsgToQue(shared_ptr<LogicNode> msg)
{
	boost::asio::post(msg->_session->GetSocket().get_executor(),
					  [this, msg]
					  {
						  auto it = _fun_callbacks.find(msg->_recvnode->_msg_id);
						  if (it == _fun_callbacks.end())
							  return;
						  it->second(msg->_session, msg->_recvnode->_msg_id, std::string(msg->_recvnode->_data, msg->_recvnode->_cur_len));
					  });
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
	root["data"] = "server has received msg, msg data is " + root["data"].asString();
	// std::string return_str = root.toStyledString();
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";

	std::string return_str =
		Json::writeString(builder, root);

	session->Send(return_str, root["id"].asInt());
}