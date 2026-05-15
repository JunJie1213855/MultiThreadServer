#include "mts/thread_pool.h"
#include <pthread.h>

namespace mts
{

ContextThreadPool::ContextThreadPool(int thread_num, bool pin_threads)
	: _next_index(0),
	  _threadNum(thread_num > 0
					 ? thread_num
					 : static_cast<int>(std::thread::hardware_concurrency()))
{
	for (int i = 0; i < _threadNum; ++i)
	{
		auto ctx = std::make_unique<boost::asio::io_context>();
		auto work = std::make_unique<boost::asio::io_context::work>(*ctx);
		_io_contexts.push_back(std::move(ctx));
		_works.push_back(std::move(work));
	}

	for (int i = 0; i < _threadNum; ++i)
	{
		_threads.emplace_back([this, i, pin_threads]()
		{
			if (pin_threads)
			{
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				CPU_SET(i, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
			}
			_io_contexts[i]->run();
		});
	}
}

ContextThreadPool::~ContextThreadPool()
{
	stop();
}

boost::asio::io_context &ContextThreadPool::next_io_context()
{
	size_t idx = _next_index.fetch_add(1, std::memory_order_relaxed) % _io_contexts.size();
	return *_io_contexts[idx];
}

void ContextThreadPool::stop()
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

} // namespace mts
