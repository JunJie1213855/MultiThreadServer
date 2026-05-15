#ifndef MTS_EXAMPLE_HANDLERS_H_
#define MTS_EXAMPLE_HANDLERS_H_

namespace mts
{
	class Server;
}

// 把 echo_server 的业务 handler 注册到 server 上。
// 必须在 server.run() 之前调用。
void register_handlers(mts::Server &server);

#endif
