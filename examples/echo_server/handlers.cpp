#include "handlers.h"
#include "messages.h"
#include "mts/server.h"
#include "mts/session.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <memory>
#include <string>

namespace
{

// 处理 HELLO_WORD 消息：回显并在 data 前加上一段前缀。
void hello_world(std::shared_ptr<mts::TCPSession> session, short /*msg_id*/, std::string msg_data)
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

} // namespace

void register_handlers(mts::TCPServer &server)
{
	server.on_message(MSG_HELLO_WORD, hello_world);
}
