#include "LogicSystem.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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
void LogicSystem::HelloWordCallBack(shared_ptr<CSession> session, short msg_id, const string& msg_data)
{
	rapidjson::Document doc;
	if (doc.Parse(msg_data.data(), msg_data.size()).HasParseError())
		return;
	if (!doc.IsObject() || !doc.HasMember("id") || !doc.HasMember("data"))
		return;

	int id = doc["id"].GetInt();

	// 构造新的 data 值并就地替换（避免 string 拼接产生临时对象）
	std::string new_data = "server has received msg, msg data is ";
	new_data += doc["data"].GetString();
	doc["data"].SetString(new_data.data(),
						  static_cast<rapidjson::SizeType>(new_data.size()),
						  doc.GetAllocator());

	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
	doc.Accept(writer);

	session->Send(std::string(sb.GetString(), sb.GetSize()), id);
}