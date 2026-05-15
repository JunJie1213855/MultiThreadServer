#ifndef MTS_THREAD_POOL_H_
#define MTS_THREAD_POOL_H_

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace mts
{

	// 一线程一 io_context 的线程池。每个 io_context 由一个专属线程驱动，
	// 可选地通过 pthread_setaffinity_np 绑定到对应 CPU 核心。
	class ContextThreadPool
	{
	public:
		// thread_num == 0 表示使用 std::thread::hardware_concurrency()
		explicit ContextThreadPool(int thread_num = 0, bool pin_threads = true);
		~ContextThreadPool();

		ContextThreadPool(const ContextThreadPool &) = delete;
		ContextThreadPool &operator=(const ContextThreadPool &) = delete;

		// 轮询返回下一个 io_context 引用（指向稳定的 vector 元素）
		boost::asio::io_context &next_io_context();
		void stop();

	private:
		std::vector<std::unique_ptr<boost::asio::io_context>> _io_contexts;
		std::vector<std::unique_ptr<boost::asio::io_context::work>> _works;
		std::vector<std::thread> _threads;
		std::atomic<size_t> _next_index;
		int _threadNum;
	};

} // namespace mts

#endif
