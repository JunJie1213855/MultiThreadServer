# mts

基于 Boost.Asio + C++20 无栈协程的可复用异步 TCP 服务器库。

采用 **一线程一 io_context（one-loop-per-thread）** 模型：一个独立的 acceptor
事件循环负责接受连接，并把每条连接轮询分配给线程池中某个绑核的 io_context。
库核心不含任何业务逻辑，使用者通过 `on_message` 注册自己的消息 handler。

## 特性

- **C++20 协程**：Boost.Asio 无栈协程（stackless coroutine）
- **一线程一 io_context**：每个 io_context 由专属线程驱动并绑定 CPU 核心
- **无单例**：`TCPServer` 拥有自己的 `ContextThreadPool` 与 `Dispatcher`，可同时跑多个实例、可嵌入、可测试
- **业务无关**：核心不依赖任何 JSON 库；消息分发完全由使用者注册的 handler 决定
- **可打包**：提供 `find_package(mts)` 的安装/导出配置，静态库 + 动态库均导出

## 目录结构

```
MultiThreadTCPServer/
├── CMakeLists.txt              # 库（静态 + 动态）+ 安装/导出
├── cmake/
│   └── mts-config.cmake.in     # find_package(mts) 配置模板
├── include/mts/                # 公开头文件（安装）
│   ├── mts.h                   # 伞形头文件
│   ├── server.h                # TCPServer
│   ├── server_config.h         # ServerConfig
│   ├── session.h               # TCPSession
│   ├── dispatcher.h            # Dispatcher（msg_id -> handler）
│   ├── thread_pool.h           # ContextThreadPool（一线程一 io_context）
│   ├── frame.h                 # 帧编解码 seam
│   └── msg_node.h              # 收发消息节点
├── src/                        # 库实现
├── examples/
│   ├── echo_server/            # 消费 mts 库的回显服务器（含 RapidJSON handler）
│   └── load_client/            # 负载测试客户端（200 线程 × 500 次往返）
└── tests/consumer/             # find_package(mts) 消费者冒烟测试
```

## 线协议

消息格式：`[2字节 msg_id][2字节 length][数据]`，`msg_id` 与 `length` 为网络字节序。

```
+--------+--------+------------+
| msg_id | length |    data    |
+--------+--------+------------+
   2B       2B        N B
```

## 构建

```bash
cmake -B build
cmake --build build -j

# 运行示例回显服务器（监听 10086）
./build/echo_server

# 运行负载测试客户端
./build/load_client
```

CMake 选项：

- `BUILD_WITH_URING=ON`（默认）— 定义 `BOOST_ASIO_HAS_IO_URING`。无 liburing 的系统请关闭。
- `BUILD_OPTIM=ON`（默认）— 加 `-O3 -march=native` 等优化标志（仅 PRIVATE 附加到库目标，不进入导出接口）。
- `MTS_BUILD_EXAMPLES`（顶层项目时默认 ON，被嵌入时默认 OFF）— 是否构建 `examples/`。

编译器要求：GCC 13+（C++20 无栈协程）。依赖：Boost ≥ 1.70（`system`、`thread`）。
示例程序额外依赖 RapidJSON（header-only）与可选的 jemalloc。

## 安装

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/your/prefix
cmake --build build -j
cmake --install build
```

安装内容：`libmts.a` + `libmts.so`、`include/mts/` 公开头文件、`lib/cmake/mts/` 下的
`find_package` 配置。

## 使用本库（Using the library）

第三方项目通过 `find_package(mts)` 引入：

```cmake
find_package(mts REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE mts::mts)   # mts::mts 即动态库；静态库为 mts::mts-static
```

注册 handler 并启动服务：

```cpp
#include "mts/mts.h"

void on_hello(std::shared_ptr<mts::TCPSession> session, short msg_id, std::string body)
{
    // ... 解析 body，处理业务，session->Send(reply, msg_id);
}

int main()
{
    mts::ServerConfig cfg;
    cfg.port = 12345;

    mts::TCPServer server(cfg);
    server.on_message(1001, on_hello);   // 契约：所有 on_message 必须在 run() 之前
    server.run();                        // 阻塞，直到 stop()
}
```

**契约**：所有 `on_message()` 注册必须 happen-before `run()`。`Dispatcher` 的 handler
表是“写一次、之后只读”，因此运行期无锁。`stop()` 可从任意线程（如信号处理线程）调用。

## CI

`.github/workflows/ci.yml` 包含三个 job：

1. **Build & Test** — GCC 13 + `-Wall -Wextra -Wpedantic` 构建，跑 `load_client` 负载冒烟，再跑 `cppcheck`。
2. **ASan + UBSan** — 同样的负载冒烟在 sanitizer 下重跑（关 io_uring 以避免与 sanitizer 冲突）。
3. **Install + consumer smoke** — 安装到临时前缀，再用 `find_package(mts)` 构建消费者 fixture，并校验导出配置不含构建树绝对路径。
