# 优化建议

> 范围：当前能跑、想再快一截 / 再干净一点的改进。CRITICAL/HIGH 级别的 bug 见 `/cpp-review` 报告，本文不重复。

## 一、热路径性能（影响 QPS / 延迟最大）

### 1. 去掉热路径上的 `std::cout`（最大收益）

`HandleRead` 每条消息打 2 行（`msg_id` / `msg_len`），`HelloWordCallBack` 每条再打 1 行。`std::cout` 内部有全局锁，多线程下会成为最强串行点。

```cpp
// CSession.cpp:173, 186  以及  LogicSystem.cpp:40
// 删除或者改成 #ifdef DEBUG_LOG ... #endif
```

单这一项，在 200×500 并发的 jsonclient 跑分里通常能拉一倍以上 QPS。

### 2. 设置 `TCP_NODELAY`（小包延迟）

现在没有关 Nagle，hello-world 这种小帧会触发延迟合并。

```cpp
// CServer::StartAcceptLoop 在 async_accept 成功后：
new_session->GetSocket().set_option(tcp::no_delay(true));
// 也可以一起加：
new_session->GetSocket().set_option(boost::asio::socket_base::keep_alive(true));
```

### 3. 读路径合并成单次 read_some + 环形缓冲

当前每条消息 2 次 `async_read`（head 4B + body NB），等于每个消息至少 2 次系统调用 + 2 次 epoll/uring 唤醒。改成一次 `async_read_some` 读到固定 buffer，然后在用户态切分，能把高频小包的 syscall 数砍半甚至更多，并且天然支持流水线（一次 read 解出多帧）。

伪代码骨架：

```cpp
char _rbuf[MAX_LENGTH * 4];
size_t _rpos = 0, _rend = 0;
while (!_b_close) {
    auto [ec, n] = co_await _socket.async_read_some(
        buffer(_rbuf + _rend, sizeof(_rbuf) - _rend), as_tuple(use_awaitable));
    if (ec) { Close(); co_return; }
    _rend += n;
    while (_rend - _rpos >= HEAD_TOTAL_LEN) {
        // 解 head；body 不够则 break；够则派发 + _rpos += frame_len
    }
    if (_rpos > 0) { memmove(_rbuf, _rbuf+_rpos, _rend-_rpos); _rend-=_rpos; _rpos=0; }
}
```

这是异步框架里收益最大的一步，但改动不小，建议测过基线再上。

### 4. 异常路径换成 `as_tuple(use_awaitable)`

`HandleRead` 的两个 `try/catch` 会在每次连接断开时抛异常 → 异常对象分配 + stack unwinding。`HandleWrite` 已经用了 `as_tuple`，统一成同一种风格：

```cpp
auto [ec, n] = co_await async_read(_socket,
    buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
    as_tuple(use_awaitable));
if (ec) { Close(); co_return; }
```

顺便把 `try/catch` 都删了，代码也更短。

### 5. 单消息分配可复用

当前每条消息都 `make_shared<RecvNode>(msg_len, msg_id)` + `make_shared<LogicNode>(...)`，至少 2 次 heap alloc + 2 次原子引用计数。`LogicNode` 实际就是 `(session, recvnode)` 二元组，可以让 `PostMsgToQue` 直接接两个参数：

```cpp
// LogicSystem.h
void PostMsgToQue(std::shared_ptr<CSession> session,
                  std::shared_ptr<RecvNode> recv);
```

`LogicNode` 类整个删掉。

进一步：`_recv_msg_node` 在长度 ≤ 上一次容量时可以原地复用，不每次 new。

### 6. `LogicSystem::_fun_callbacks` 用数组或 `unordered_map`

`std::map<short, FunCallBack>`（红黑树）每次派发都是 O(log n)。msg_id 集中在一个小段（1000~），直接：

```cpp
std::array<FunCallBack, MSG_ID_MAX - MSG_ID_MIN> _fun_callbacks;
// 派发：
auto& cb = _fun_callbacks[msg_id - MSG_ID_MIN - 1];
if (cb) cb(session, msg_id, std::move(data));
```

顺便在 `RegisterCallBacks` 里把 `std::bind` 换成 lambda。

### 7. Singleton::GetInstance 返回引用

现在每次调用都返回 `shared_ptr<T>` → 拷贝时一次原子加。`LogicSystem::GetInstance()` 在每条消息都被调一次。改成静态局部 + 引用：

```cpp
static T& GetInstance() {
    static T instance;
    return instance;
}
```

代价是丢掉"延迟构造 + 共享所有权"的语义，但这两个 singleton 都是进程级生命周期，完全不需要 `shared_ptr`。

### 8. `_next_index.fetch_add` 用 relaxed

`AsioThreadPool::GetNextIOService` 里：

```cpp
size_t idx = _next_index.fetch_add(1, std::memory_order_relaxed) % _io_contexts.size();
```

这里只是个轮询计数器，不需要 seq_cst。

## 二、JSON 层（真实业务负载）

`HelloWordCallBack` 里 `Json::Reader` 解析 + `Json::Value` 构造 + `Json::writeString` 输出，单条消息至少 3~4 次 string 分配。如果以后要压性能，要么换 `simdjson` / `rapidjson`（解析快一个数量级），要么直接用二进制协议（msg_id 已经是整型了，data 段用裸 bytes）。短期最简单的 micro-opt：

```cpp
// 复用 reader 实例
thread_local Json::CharReaderBuilder builder;
thread_local Json::StreamWriterBuilder writer_builder = []{
    Json::StreamWriterBuilder b; b["indentation"] = ""; return b;
}();
```

## 三、架构层面

### 9. 优雅关闭

`main_server.cpp` 里 `signal_io_ctx.run()` 退出后，`CServer s` 还在栈上，`pool->Stop()` 已经把工作线程 join 掉了，但此时 `_acceptor_thread` 仍在 `_io_context.run()`。CServer 析构里再 stop+join，顺序还行；但**正在跑的 session 协程被强行打断**，`_send_que` 里没发完的消息直接丢。

合理的关停顺序：

1. `_acceptor.close()` → 不再接新连接
2. 等所有已有 session `_send_que.empty()` 后再 close socket（或者给个超时）
3. `pool->Stop()`

### 10. 背压策略

`Send` 里 `_send_que > MAX_SENDQUE` 直接 `cout` + `return` —— **静默丢消息**。两种更好的做法：

- **关连接**：`Close()`，让客户端感知到。
- **流控**：暂停 `HandleRead`（把 `co_await` 卡在一个条件变量/timer 上），等队列降下去再恢复。

不能允许"对端以为发出去了，其实掉了"。

### 11. 空闲超时 / 半开连接清理

现在没有 idle timeout，客户端崩了不发 RST 的话，server 这边的 session 就一直挂着。给 `HandleRead` 套一个：

```cpp
boost::asio::steady_timer idle_timer(_socket.get_executor());
idle_timer.expires_after(60s);
// 每收到一条消息 reset；超时则 Close()
```

### 12. 配置化

端口 `10086`、线程数 `hardware_concurrency()`、队列上限、超时全部硬编码。最起码读个 `config.json` 或者 `argv`。

### 13. 日志框架

`std::cout` 全局锁 + 没有 level 控制。换 spdlog（async sink）+ release 关掉 debug 级。和第 1 条配合。

### 14. 监控/指标

README 写了 QPS 统计，但当前代码里**已经被删干净了**（没看到 atomic counter 也没看到统计线程）。如果还需要这功能，加一组 `std::atomic<uint64_t>` 计数器和一个独立的统计协程即可。

## 四、构建优化

### 15. CMake 加 LTO / PGO 选项

```cmake
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_ok)
if(lto_ok AND BUILD_OPTIM)
    set_property(TARGET ThreadPoolServer PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
```

LTO 对内联跨翻译单元的小函数（`MsgNode` 构造、`Send` 转发）效果明显。

### 16. CPU 亲和性策略

现在 `pthread_setaffinity_np` 把线程 i 绑到核 i。在有 SMT（超线程）的机器上，0/1 是同一物理核，会内部竞争。建议跳着绑物理核（i*2），或者干脆只起 `physical_cores` 个线程。

---

## 优先级建议

如果只想做"性价比最高的三件事"：

1. **删除 `HandleRead` / `HelloWordCallBack` 里的 `cout`**（单行改动，QPS 翻倍级）
2. **设置 `TCP_NODELAY`**（单行改动，小包延迟立刻好看）
3. **`HandleRead` 改用 `as_tuple` 移除 try/catch**（统一风格 + 异常路径变快）

中期值得做的：读端环形缓冲（#3）、移除 `LogicNode` / 对象池化（#5）、优雅关闭（#9）、背压策略（#10）。

剩下的属于"更工程化"的功夫，要不要做取决于这是不是要长期维护的项目。
