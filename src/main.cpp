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
	try {
		auto pool = AsioThreadPool::GetInstance();
		boost::asio::io_context io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([pool,&io_context](auto, auto) {
			io_context.stop();
			pool->Stop();
			});

		CServer s(pool->GetIOService(), 10086);
		
		//Æô¶¯¼àÌýÍË³öµÄio_context
		io_context.run();
		std::cout << "server exited " << std::endl;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
	}

}