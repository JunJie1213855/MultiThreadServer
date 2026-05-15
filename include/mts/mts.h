#ifndef MTS_MTS_H_
#define MTS_MTS_H_

// mts —— 可复用的异步 TCP 服务器库（一线程一 io_context，C++20 协程）。
//
// 用法：
//   mts::ServerConfig cfg; cfg.port = 12345;
//   mts::TCPServer server(cfg);
//   server.on_message(MY_MSG_ID, my_handler);   // 必须在 run() 之前注册
//   server.run();                               // 阻塞，直到 stop()

#include "mts/server_config.h"
#include "mts/frame.h"
#include "mts/msg_node.h"
#include "mts/thread_pool.h"
#include "mts/dispatcher.h"
#include "mts/session.h"
#include "mts/server.h"

#endif
