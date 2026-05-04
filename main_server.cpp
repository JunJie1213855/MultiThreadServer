#include <iostream>
#include "CServer.h"
#include "Singleton.h"
#include "LogicSystem.h"
#include <csignal>
#include "AsioThreadPool.h"
using namespace std;

int main()
{
	try
	{
		// 初始化 Asio 线程池（会在内部创建多个 io_context）
		auto pool = AsioThreadPool::GetInstance();

		// 创建独立的 io_context 用于信号处理
		boost::asio::io_context signal_io_ctx;

		// 设置信号处理（Ctrl+C 退出）
		boost::asio::signal_set signals(signal_io_ctx, SIGINT, SIGTERM);
		signals.async_wait([pool](auto, auto)
						   {
			pool->Stop();
			std::cout << "Bye" << std::endl; });

		// 创建服务器，监听 10086 端口（acceptor 运行在独立线程）
		CServer s(10086);

		// 运行信号监听事件循环
		signal_io_ctx.run();

		std::cout << "server exited " << std::endl;
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << endl;
	}
}