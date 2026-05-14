#ifndef CSERVER_H_
#define CSERVER_H_

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include "CSession.h"
#include "mts/thread_pool.h"
#include "mts/dispatcher.h"
#include <memory>
#include <map>
#include <mutex>
#include <thread>
using boost::asio::ip::tcp;

class CServer
{
public:
	CServer(short port);
	~CServer();
	boost::asio::io_context &GetIOContext() { return _io_context; }
	void ClearSession(std::string);

	// 注册消息 handler。契约：所有 on_message 必须 happen-before 服务开始收消息。
	void on_message(short msg_id, mts::Dispatcher::Handler handler);
	mts::Dispatcher &dispatcher() noexcept { return dispatcher_; }

private:
	boost::asio::awaitable<void> StartAcceptLoop();

private:
	boost::asio::io_context _io_context;
	short _port;
	tcp::acceptor _acceptor;
	// pool_ 在 _sessions 之前声明：析构时 _sessions 先释放（放掉 CServer 持有的
	// session 引用），pool_ 后析构（其 io_context 被销毁时丢弃残留的 handler）。
	mts::ThreadPool pool_;
	mts::Dispatcher dispatcher_;
	std::map<std::string, std::shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
	std::thread _acceptor_thread;
};

#endif