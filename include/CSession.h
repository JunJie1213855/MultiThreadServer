#ifndef CSESSION_H_
#define CSESSION_H_

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include "const.h"
#include "MsgNode.h"
using namespace std;

using boost::asio::ip::tcp;
class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context &io_context, CServer *server);
	~CSession();
	tcp::socket &GetSocket();
	std::string &GetUuid();
	void Start();
	void StartCoroutine();
	void Send(char *msg, short max_length, short msgid);
	void Send(std::string msg, short msgid);
	void Close();
	std::shared_ptr<CSession> SharedSelf();

private:
	boost::asio::awaitable<void> HandleRead();
	boost::asio::awaitable<void> HandleWrite();
	// boost::asio::awaitable<void> ProcessMessage();

private:
	tcp::socket _socket;
	std::string _uuid;
	char _data[MAX_LENGTH];
	CServer *_server;
	bool _b_close;
	std::queue<shared_ptr<SendNode>> _send_que;
	std::mutex _send_lock;
	std::shared_ptr<RecvNode> _recv_msg_node;
	bool _b_head_parse;
	std::shared_ptr<MsgNode> _recv_head_node;
};

class LogicNode
{
	friend class LogicSystem;

public:
	LogicNode(shared_ptr<CSession>, shared_ptr<RecvNode>);

private:
	shared_ptr<CSession> _session;
	shared_ptr<RecvNode> _recvnode;
};

#endif