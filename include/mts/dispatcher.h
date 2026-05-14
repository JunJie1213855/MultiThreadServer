#ifndef MTS_DISPATCHER_H_
#define MTS_DISPATCHER_H_

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>

class CSession;
class RecvNode;

namespace mts
{

	// msg_id -> handler 的注册与分发表。
	//
	// 契约：所有 on() 注册必须 happen-before 服务开始收消息（Phase 4 的 run()）。
	// handlers_ 是“写一次、之后只读”，因此无需加锁。
	class Dispatcher
	{
	public:
		using Handler = std::function<void(std::shared_ptr<CSession>, short, std::string)>;

		void on(short msg_id, Handler h);

		// 把消息投递到 session 自己的 executor 上执行对应 handler；
		// 未注册的 msg_id 直接忽略（无害的 no-op）。
		void dispatch(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);

	private:
		std::unordered_map<short, Handler> handlers_;
	};

} // namespace mts

#endif
