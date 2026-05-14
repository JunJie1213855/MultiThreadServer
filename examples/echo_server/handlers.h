#ifndef MTS_EXAMPLE_HANDLERS_H_
#define MTS_EXAMPLE_HANDLERS_H_

class CServer;

// 把 echo_server 的业务 handler 注册到 server 上。
// 必须在 server 开始收消息之前调用。
void register_handlers(CServer &server);

#endif
