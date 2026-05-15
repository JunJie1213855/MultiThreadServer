#ifndef MTS_SERVER_CONFIG_H_
#define MTS_SERVER_CONFIG_H_

#include <cstddef>

namespace mts
{

	// TCPServer 的构造参数。除 port 外均有合理默认值。
	struct ServerConfig
	{
		unsigned short port = 0;			  // 必填：监听端口
		int io_threads = 0;					  // io 线程数；0 => hardware_concurrency()
		std::size_t max_msg_length = 2048;	  // 单条消息体长度上限（原 MAX_LENGTH）
		std::size_t max_send_queue = 1000;	  // 发送队列深度上限（原 MAX_SENDQUE）
		bool pin_threads = true;			  // 是否 pthread_setaffinity_np 绑核
		bool tcp_no_delay = true;			  // 是否对新连接设置 TCP_NODELAY
	};

} // namespace mts

#endif
