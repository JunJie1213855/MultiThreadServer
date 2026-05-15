#ifndef MTS_SESSION_H_
#define MTS_SESSION_H_

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <memory>
#include "mts/msg_node.h"

namespace mts
{

	using boost::asio::ip::tcp;
	class Server;

	// 一条 TCP 连接。生命周期与并发模型：
	//   - enable_shared_from_this，靠 co_spawn/post 里的 shared_from_this() 捕获续命；
	//   - 持有裸 Server* 回指针；
	//   - _send_que / _b_close 只在本 session 自己的 executor 上访问，因此无锁。
	// 跨线程调用者必须把操作 post 到本 session 的 executor 上；同线程调用可直接访问。
	class Session : public std::enable_shared_from_this<Session>
	{
	public:
		Session(boost::asio::io_context &io_context, Server *server);
		~Session();
		tcp::socket &GetSocket();
		std::string &GetUuid();
		void Start();
		void Send(char *msg, short max_length, short msgid);
		void Send(std::string msg, short msgid);
		void Close();
		std::shared_ptr<Session> SharedSelf();

	private:
		boost::asio::awaitable<void> HandleRead();
		boost::asio::awaitable<void> HandleWrite();

	private:
		tcp::socket _socket;
		std::string _uuid;
		Server *_server;
		bool _b_close;
		std::queue<std::shared_ptr<SendNode>> _send_que;
		std::shared_ptr<RecvNode> _recv_msg_node;
		bool _b_head_parse;
		std::shared_ptr<MsgNode> _recv_head_node;
	};

} // namespace mts

#endif
