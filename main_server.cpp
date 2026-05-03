#include <iostream>
#include "CServer.h"
#include "Singleton.h"
#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioThreadPool.h"
using namespace std;

int main()
{
	try
	{
		std::atomic<bool> is_running{true};

		// 初始化 Asio 线程池（会在内部创建多个 io_context）
		auto pool = AsioThreadPool::GetInstance();

		// 创建独立的 io_context 用于信号处理
		boost::asio::io_context signal_io_ctx;

		// 设置信号处理（Ctrl+C 退出）
		boost::asio::signal_set signals(signal_io_ctx, SIGINT, SIGTERM);
		signals.async_wait([pool, &is_running](auto, auto)
						   {
			pool->Stop();
			is_running.store(false);
			std::cout << "Bye" << std::endl; });

		// 创建服务器，监听 10086 端口（acceptor 运行在独立线程）
		CServer s(10086);

		// 启动统计打印线程（每5秒打印一次）
		std::thread stats_thread([&is_running]()
								 {
			while (is_running.load(std::memory_order::relaxed))
			{
				std::this_thread::sleep_for(std::chrono::seconds(5));
				auto logic = LogicSystem::GetInstance();
				std::cout << "[QPS] " << logic->GetQPS()
						  << " [Total] " << logic->GetTotalRequests()
						  << " [ResponseRate] " << logic->GetResponseRate() << "%"
						  << std::endl;
			} });

		// 运行信号监听事件循环
		signal_io_ctx.run();

		// 打印最终统计
		LogicSystem::GetInstance()->PrintStatistics();

		// 等待统计线程结束
		if (stats_thread.joinable())
			stats_thread.join();
		std::cout << "server exited " << std::endl;
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << endl;
	}
}