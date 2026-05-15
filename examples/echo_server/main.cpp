#include <iostream>
#include <csignal>
#include <thread>
#include "mts/mts.h"
#include "handlers.h"
using namespace std;

int main()
{
	try
	{
		mts::ServerConfig cfg;
		cfg.port = 10086;

		// 先创建 Server（其构造会建立线程池的工作线程）。signal_set 必须在
		// 工作线程之后构造 —— 否则可能拦不到 SIGINT/SIGTERM。
		mts::Server s(cfg);

		// 注册业务消息 handler（必须在 s.run() 之前完成）
		register_handlers(s);

		// 信号处理（Ctrl+C / SIGTERM）跑在独立线程上，handler 调用 s.stop()，
		// 让阻塞中的 s.run() 返回。
		boost::asio::io_context signal_ctx;
		boost::asio::signal_set signals(signal_ctx, SIGINT, SIGTERM);
		signals.async_wait([&s](auto, auto)
						   {
			s.stop();
			std::cout << "Bye" << std::endl; });
		std::thread signal_thread([&signal_ctx]
								  { signal_ctx.run(); });

		// 阻塞运行，直到 stop()
		s.run();

		signal_ctx.stop();
		signal_thread.join();
		std::cout << "server exited " << std::endl;
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << endl;
	}
}
