#include "CServer.h"
#include "CSession.h"
#include <iostream>

using namespace boost::asio;

CServer::CServer(short port)
    : _port(port), _acceptor(_io_context, tcp::endpoint(tcp::v4(), port))
{
    std::cout << "Server start success, listen on port : " << _port << std::endl;
    _acceptor.listen();
    co_spawn(_io_context, [this]
             { return StartAcceptLoop(); }, detached);
    _acceptor_thread = std::thread([this]()
                                   { _io_context.run(); });
}

CServer::~CServer()
{
    // 先停掉 acceptor 与线程池并 join 所有线程，确保所有成员析构前已无运行中的协程。
    _io_context.stop();
    if (_acceptor_thread.joinable())
        _acceptor_thread.join();
    pool_.stop();
    std::cout << "Server destruct listen on port : " << _port << std::endl;
}

awaitable<void> CServer::StartAcceptLoop()
{
    for (;;)
    {
        auto &io_ctx = pool_.next_io_context();
        auto new_session = std::make_shared<CSession>(io_ctx, this);
        // auto &socket = new_session->GetSocket();

        auto [ec] = co_await _acceptor.async_accept(new_session->GetSocket(),
                                                    boost::asio::as_tuple(use_awaitable));
        if (ec == boost::asio::error::operation_aborted)
            co_return;
        if (ec)
        {
            std::cerr << "accept error" << ec.message() << std::endl;
            continue;
        }
        boost::system::error_code opt_ec;
        new_session->GetSocket().set_option(tcp::no_delay(true), opt_ec);
        new_session->Start();
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _sessions.insert(make_pair(new_session->GetUuid(), new_session));
        }
    }
}

void CServer::ClearSession(std::string uuid)
{
    // std::lock_guard<std::mutex> lock(_mutex);
    boost::asio::post(_acceptor.get_executor(), [this, uuid = std::move(uuid)]
                      { this->_sessions.erase(uuid); });
}

void CServer::on_message(short msg_id, mts::Dispatcher::Handler handler)
{
    dispatcher_.on(msg_id, std::move(handler));
}