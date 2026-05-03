# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
mkdir -p build && cd build && cmake .. && make
```

Targets: `ThreadPoolServer` (server), `main_jsonclient` (test client)

## Architecture

**Boost.Asio + C++20 stackless coroutine TCP server** with per-session affinity.

```
main_server.cpp
    ├── AsioThreadPool (singleton)     // Multiple io_contexts, each in own thread
    │   ├── _io_contexts[0] ── thread 0
    │   ├── _io_contexts[1] ── thread 1
    │   └── ...
    ├── CServer (acceptor thread)      // Own io_context for listening
    │   └── StartAcceptLoop()          // round-robins to pool for new sessions
    └── CSession (per-session)         // Each session bound to ONE io_context
        ├── HandleRead()
        └── HandleWrite()
```

**Key design**: Thread pool (`AsioThreadPool`) manages N `io_context` instances (N = hardware concurrency). `CServer`'s acceptor runs on a dedicated thread. When a new connection is accepted, it is assigned to an `io_context` via round-robin (`GetNextIOService()`). All async operations for that session run on the same `io_context`/thread, benefiting CPU cache locality.

**Session assignment flow**:
1. `CServer::StartAcceptLoop()` accepts connection
2. Calls `pool->GetNextIOService()` to get next `io_context`
3. Creates `CSession` with that `io_context`
4. Session's `HandleRead()`/`HandleWrite()` coroutines run on that dedicated thread

## Message Protocol

`[2 bytes msg_id][2 bytes length][data]`

- Header: `HEAD_TOTAL_LEN = 4` bytes (`HEAD_ID_LEN=2` + `HEAD_DATA_LEN=2`)
- msg_id: network byte order, valid range `1001` (`MSG_HELLO_WORD`)
- msg_len: body size, max `MAX_LENGTH = 2048`

## Key Files

| File | Purpose |
|------|---------|
| `include/AsioThreadPool.h` | Thread pool — owns multiple `io_context`s, `GetNextIOService()` for round-robin |
| `include/CSession.h` | Per-client session; `HandleRead()`/`HandleWrite()` coroutines |
| `include/CServer.h` | Acceptor — runs `StartAcceptLoop()` on dedicated thread |
| `include/const.h` | Protocol constants and `MSG_IDS` enum |
| `src/CSession.cpp` | `HandleRead()` and `HandleWrite()` coroutine logic |
| `BugReport.md` | **Known bugs** — read before making changes |

## Constants (const.h)

```cpp
MAX_LENGTH = 2048       // max body size
HEAD_TOTAL_LEN = 4      // header size in bytes
HEAD_ID_LEN = 2         // msg_id field size
HEAD_DATA_LEN = 2       // msg_len field size
MAX_SENDQUE = 1000      // send queue high watermark
MSG_ID_MIN = 1000
MSG_HELLO_WORD = 1001
MSG_ID_MAX = ...
```

## Critical Patterns

**Starting a session coroutine:**
```cpp
void CSession::Start() {
    co_spawn(_socket.get_executor(), [self = shared_from_this()]
             { return self->HandleRead(); }, detached);
}
```

**Round-robin session assignment:**
```cpp
auto &io_ctx = pool->GetNextIOService();
auto new_session = std::make_shared<CSession>(io_ctx, this);
```

**Send queue pattern** (lock + check + spawn):
```cpp
void CSession::Send(std::string msg, short msgid) {
    lock_guard<mutex> lock(_send_lock);
    bool need_spawn = _send_que.empty();
    _send_que.push(make_shared<SendNode>(...));
    if (need_spawn) {
        co_spawn(_socket.get_executor(), [self = shared_from_this()]
                 { return self->HandleWrite(); }, detached);
    }
}
```

## Known Issues

See `BugReport.md` for documented bugs. Known critical issues:
1. `HandleWrite` uses `use_awaitable` but checks `error_code` — error handling is broken
2. Send queue race: `front()` then `pop()` outside lock in `HandleWrite`
3. `stats_thread` has no退出机制, causes `join()` to hang
4. `msg_id` validation uses `MAX_LENGTH` instead of valid range check

## Dependencies

- Boost >= 1.70 (system, thread components)
- JsonCpp
- C++20 compiler with `-fcoroutines` (GCC 13+)