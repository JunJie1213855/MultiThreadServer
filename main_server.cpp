#include <iostream>
#include "CServer.h"
#include "handlers.h"
#include <csignal>
using namespace std;

int main()
{
	try
	{
		// 先创建服务器，监听 10086 端口（acceptor 运行在独立线程,
		// 线程池由 CServer 持有）。在 signal_set 之前构造，使所有 io 工作线程
		// 早于信号注册建立 —— 否则 signal_set 可能拦不到 SIGINT/SIGTERM。
		CServer s(10086);

		// 注册业务消息 handler（必须在客户端开始发消息之前完成）
		register_handlers(s);

		// 信号处理（Ctrl+C 退出）：收到信号后停掉 acceptor 的 io_context，
		// signal_io_ctx.run() 随即返回，CServer 析构时统一停止 acceptor 与线程池。
		// 完整的 run()/stop() API 在 Phase 4 落地。
		boost::asio::io_context signal_io_ctx;
		boost::asio::signal_set signals(signal_io_ctx, SIGINT, SIGTERM);
		signals.async_wait([&s](auto, auto)
						   {
			s.GetIOContext().stop();
			std::cout << "Bye" << std::endl; });

		// 运行信号监听事件循环
		signal_io_ctx.run();

		std::cout << "server exited " << std::endl;
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << endl;
	}
}
