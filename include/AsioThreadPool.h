#ifndef ASIOTHREADPOOL_H_
#define ASIOTHREADPOOL_H_

#include <boost/asio.hpp>
#include "Singleton.h"
class AsioThreadPool : public Singleton<AsioThreadPool>
{
	friend class Singleton<AsioThreadPool>;

public:
	~AsioThreadPool();
	AsioThreadPool &operator=(const AsioThreadPool &) = delete;
	AsioThreadPool(const AsioThreadPool &) = delete;
	boost::asio::io_context &GetIOService();
	boost::asio::io_context &GetIOService(int idx);
	void Stop();
	int GetThreadCount() const { return _thread_count; }

private:
	AsioThreadPool(int thread_count = std::thread::hardware_concurrency());

private:
	int _thread_count;
	int _idx;
	std::mutex _mtx;
	std::vector<std::unique_ptr<boost::asio::io_context>> _contexts;
	std::vector<std::unique_ptr<boost::asio::io_context::work>> _works;
	std::vector<std::thread> _threads;
};

#endif
