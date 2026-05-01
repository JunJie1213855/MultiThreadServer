#include "AsioThreadPool.h"

AsioThreadPool::~AsioThreadPool()
{
    Stop();
}

AsioThreadPool::AsioThreadPool(int thread_count)
    : _thread_count(thread_count), _idx(0)
{
    for (int i = 0; i < thread_count; ++i)
    {
        auto ctx = std::make_unique<boost::asio::io_context>();
        auto work = std::make_unique<boost::asio::io_context::work>(*ctx);
        _contexts.push_back(std::move(ctx));
        _works.push_back(std::move(work));
    }

    for (int i = 0; i < thread_count; ++i)
    {
        _threads.emplace_back([this, i]()
                              { _contexts[i]->run(); });
    }
}

boost::asio::io_context &AsioThreadPool::GetIOService()
{
    std::lock_guard<std::mutex> lock(_mtx);
    int index = _idx++ % _thread_count;
    return *_contexts[index];
}

boost::asio::io_context &AsioThreadPool::GetIOService(int idx)
{
    return *_contexts[idx % _thread_count];
}

void AsioThreadPool::Stop()
{
    for (auto &work : _works)
    {
        work.reset();
    }
    for (auto &ctx : _contexts)
    {
        ctx->stop();
    }
    for (auto &t : _threads)
    {
        if (t.joinable())
            t.join();
    }
}