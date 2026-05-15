#include "mts/server.h"
#include "mts/session.h"
#include <iostream>

namespace mts
{

	using boost::asio::co_spawn;
	using boost::asio::detached;
	using boost::asio::use_awaitable;
	using boost::asio::ip::tcp;

	Server::Server(ServerConfig config)
		: config_(config),
		  acceptor_(acceptor_ctx_, tcp::endpoint(tcp::v4(), config_.port)),
		  pool_(config_.io_threads, config_.pin_threads)
	{
		std::cout << "Server start success, listen on port : " << config_.port << std::endl;
		acceptor_.listen();
	}

	Server::~Server()
	{
		// stop() 是幂等的：确保 acceptor io_context 与线程池都已停止、
		// 所有线程都已 join，再让成员开始析构。
		stop();
		std::cout << "Server destruct listen on port : " << config_.port << std::endl;
	}

	void Server::run()
	{
		co_spawn(acceptor_ctx_, [this]
				 { return accept_loop(); }, detached);
		// 在调用者线程上跑 acceptor 的 io_context，阻塞直到 stop()。
		acceptor_ctx_.run();
	}

	void Server::stop()
	{
		// io_context::stop() 线程安全，可从任意线程（含信号处理线程）调用，
		// 它会让 run() 尽快返回。监听 socket 在 acceptor_ 成员析构时释放 ——
		// 此处不跨线程 close acceptor，避免与 accept_loop 竞争。
		acceptor_ctx_.stop();
		pool_.stop();
	}

	boost::asio::awaitable<void> Server::accept_loop()
	{
		for (;;)
		{
			auto &io_ctx = pool_.next_io_context();
			auto new_session = std::make_shared<Session>(io_ctx, this);

			auto [ec] = co_await acceptor_.async_accept(new_session->GetSocket(),
														boost::asio::as_tuple(use_awaitable));
			if (ec == boost::asio::error::operation_aborted)
				co_return;
			if (ec)
			{
				std::cerr << "accept error" << ec.message() << std::endl;
				continue;
			}
			if (config_.tcp_no_delay)
			{
				boost::system::error_code opt_ec;
				new_session->GetSocket().set_option(tcp::no_delay(true), opt_ec);
			}
			new_session->Start();
			{
				std::lock_guard<std::mutex> lock(mutex_);
				sessions_.insert(std::make_pair(new_session->GetUuid(), new_session));
			}
		}
	}

	void Server::clear_session(const std::string &uuid)
	{
		boost::asio::post(acceptor_.get_executor(), [this, uuid]
						  { sessions_.erase(uuid); });
	}

	void Server::on_message(short msg_id, Dispatcher::Handler handler)
	{
		dispatcher_.on(msg_id, std::move(handler));
	}

} // namespace mts
