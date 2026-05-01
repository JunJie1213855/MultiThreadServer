#include "CServer.h"
#include "CSession.h"
#include <iostream>
#include "AsioThreadPool.h"

using namespace boost::asio;

CServer::CServer(boost::asio::io_context &io_context, short port)
    : _io_context(io_context)
    , _port(port)
    , _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
    std::cout << "Server start success, listen on port : " << _port << std::endl;
    co_spawn(_io_context,
        [this]{ return StartAcceptLoop(); },
        detached);
}

CServer::~CServer()
{
    std::cout << "Server destruct listen on port : " << _port << std::endl;
}

awaitable<void> CServer::StartAcceptLoop()
{
    for (;;)
    {
        try
        {
            auto new_session = std::make_shared<CSession>(_io_context, this);
            auto &socket = new_session->GetSocket();

            co_await _acceptor.async_accept(socket, use_awaitable);

            new_session->Start();

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

void CServer::ClearSession(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(uuid);
}