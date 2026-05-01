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
		// 获取 Asio 线程池单例
		auto pool = AsioThreadPool::GetInstance();

		// 创建 io_context
		boost::asio::io_context io_context;

		// 设置信号处理（Ctrl+C 退出）
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([pool, &io_context, &is_running](auto, auto)
						   {
			io_context.stop();
			pool->Stop(); 
		is_running.store(false); });

		// 创建服务器，监听 10086 端口
		CServer s(pool->GetIOService(), 10086);

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

		// 运行事件循环
		io_context.run();

		// 打印最终统计
		LogicSystem::GetInstance()->PrintStatistics();

		// 等待统计线程结束
		stats_thread.join();
		std::cout << "server exited " << std::endl;
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << endl;
	}
}