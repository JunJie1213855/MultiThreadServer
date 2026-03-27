#include "CServer.h"
#include "CSession.h"
#include <iostream>
#include "AsioThreadPool.h"

using namespace boost::asio;

// 构造函数，创建 acceptor 并启动 accept 协程
CServer::CServer(boost::asio::io_context &io_context, short port) : _io_context(io_context), _port(port),
																	_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
	std::cout << "Server start success, listen on port : " << _port << std::endl;
	co_spawn(_io_context, StartAcceptLoop(), detached);
}

CServer::~CServer()
{
	std::cout << "Server destruct listen on port : " << _port << std::endl;
}

// 协程：accept 循环，处理新连接
awaitable<void> CServer::StartAcceptLoop()
{
	while (true)
	{
		try
		{
			// 创建新的 session
			auto new_session = std::make_shared<CSession>(_io_context, this);
			auto &socket = new_session->GetSocket();

			// 协程方式 accept
			co_await _acceptor.async_accept(socket, use_awaitable);

			// 启动 session
			new_session->Start();

			// 保存 session
			std::lock_guard<std::mutex> lock(_mutex);
			_sessions.insert(make_pair(new_session->GetUuid(), new_session));
		}
		catch (const boost::system::system_error &ec)
		{
			std::cout << "session accept failed, error is " << ec.what() << std::endl;
		}
		catch (std::exception &e)
		{
			std::cerr << "Accept exception: " << e.what() << std::endl;
		}
	}
}

// 移除指定的 session
void CServer::ClearSession(std::string uuid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_sessions.erase(uuid);
}