#ifndef CSERVER_H_
#define CSERVER_H_

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include "CSession.h"
#include <memory>
#include <map>
#include <mutex>
using namespace std;
using boost::asio::ip::tcp;

class CServer
{
public:
	CServer(boost::asio::io_context &io_context, short port);
	~CServer();
	void ClearSession(std::string);

private:
	boost::asio::awaitable<void> StartAcceptLoop();

private:
	boost::asio::io_context &_io_context;
	short _port;
	tcp::acceptor _acceptor;
	std::map<std::string, shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
};

#endif