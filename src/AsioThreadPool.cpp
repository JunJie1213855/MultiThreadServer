#include "AsioThreadPool.h"
#include <iostream>

AsioThreadPool::AsioThreadPool(int threadNum) : _next_index(0), _threadNum(threadNum)
{
	for (int i = 0; i < threadNum; ++i)
	{
		auto ctx = std::make_unique<boost::asio::io_context>();
		auto work = std::make_unique<boost::asio::io_context::work>(*ctx);
		_io_contexts.push_back(std::move(ctx));
		_works.push_back(std::move(work));
	}

	for (int i = 0; i < threadNum; ++i)
	{
		_threads.emplace_back([this, i]()
		{
			_io_contexts[i]->run();
		});
	}
}

AsioThreadPool::~AsioThreadPool()
{
	Stop();
}

boost::asio::io_context &AsioThreadPool::GetNextIOService()
{
	size_t idx = _next_index.fetch_add(1) % _io_contexts.size();
	return *_io_contexts[idx];
}

void AsioThreadPool::Stop()
{
	for (auto &work : _works)
	{
		work.reset();
	}
	for (auto &ctx : _io_contexts)
	{
		ctx->stop();
	}
	for (auto &t : _threads)
	{
		if (t.joinable())
			t.join();
	}
}