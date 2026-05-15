#ifndef MTS_SERVER_H_
#define MTS_SERVER_H_

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include "mts/session.h"
#include "mts/thread_pool.h"
#include "mts/dispatcher.h"
#include "mts/server_config.h"
#include <memory>
#include <map>
#include <mutex>
#include <string>

namespace mts
{

	// 异步 TCP 服务器。拥有线程池与消息分发器（均非单例）。
	//
	// 典型用法：
	//   mts::ServerConfig cfg; cfg.port = 12345;
	//   mts::Server s(cfg);
	//   s.on_message(id, handler);   // 必须在 run() 之前
	//   s.run();                     // 阻塞
	class Server
	{
	public:
		explicit Server(ServerConfig config);
		~Server();
		Server(const Server &) = delete;
		Server &operator=(const Server &) = delete;

		// 注册消息 handler。契约：所有 on_message 必须 happen-before run()。
		void on_message(short msg_id, Dispatcher::Handler handler);

		// 阻塞：在调用者线程上运行 acceptor 的 io_context，直到 stop()。
		void run();
		// 可从任意线程调用（含信号处理线程），让 run() 尽快返回并停止线程池。
		void stop();

		// 以下供 Session 内部使用
		void clear_session(const std::string &uuid);
		Dispatcher &dispatcher() noexcept { return dispatcher_; }
		const ServerConfig &config() const noexcept { return config_; }

	private:
		boost::asio::awaitable<void> accept_loop();

		ServerConfig config_;
		boost::asio::io_context acceptor_ctx_;
		boost::asio::ip::tcp::acceptor acceptor_;
		// pool_ 在 sessions_ 之前声明：析构时 sessions_ 先释放（放掉 Server 持有的
		// session 引用），pool_ 后析构（其 io_context 被销毁时丢弃残留的 handler）。
		ThreadPool pool_;
		Dispatcher dispatcher_;
		std::map<std::string, std::shared_ptr<Session>> sessions_;
		std::mutex mutex_;
	};

} // namespace mts

#endif
