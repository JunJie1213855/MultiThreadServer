#ifndef ASIOTHREADPOOL_H_
#define ASIOTHREADPOOL_H_

#include <boost/asio.hpp>
#include "Singleton.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

class AsioThreadPool : public Singleton<AsioThreadPool>
{
	friend class Singleton<AsioThreadPool>;

public:
	~AsioThreadPool();
	AsioThreadPool &operator=(const AsioThreadPool &) = delete;
	AsioThreadPool(const AsioThreadPool &) = delete;

	boost::asio::io_context &GetNextIOService();
	void Stop();

private:
	AsioThreadPool(int threadNum = std::thread::hardware_concurrency());

private:
	std::vector<std::unique_ptr<boost::asio::io_context>> _io_contexts;
	std::vector<std::unique_ptr<boost::asio::io_context::work>> _works;
	std::vector<std::thread> _threads;
	std::atomic<size_t> _next_index;
	int _threadNum;
};

#endif