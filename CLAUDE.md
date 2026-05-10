# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
# Configure + build (out-of-tree)
mkdir -p build && cd build
cmake ..
make -j

# Run server (listens on hardcoded port 10086)
./ThreadPoolServer

# Run load-test client (200 threads × 500 round-trips against 127.0.0.1:10086)
./main_jsonclient
```

CMake options (defined in `CMakeLists.txt`):
- `BUILD_WITH_URING=ON` (default) — defines `BOOST_ASIO_HAS_IO_URING`. Disable on systems without liburing.
- `BUILD_OPTIM=ON` (default) — adds `-O3 -march=native -ffast-math -ftree-vectorize`. Disable for debug builds.

Compiler requirements: GCC 13+ (the build forces `-fcoroutines` for GCC; C++20 stackless coroutines are required throughout). Dependencies: Boost ≥ 1.70 (`system`, `thread`), JsonCpp, optional jemalloc (auto-detected via `find_library`).

There is no test target wired up in CMake — `main_jsonclient` is the de-facto load test.

## Architecture

This is a TCP server built on Boost.Asio C++20 stackless coroutines, organized around the **one-loop-per-thread** pattern. Understanding the threading model is essential before changing any session/IO code.

### Threading model

- `AsioThreadPool` (singleton, `src/AsioThreadPool.cpp`) creates one `io_context` per hardware thread, each owned by a dedicated `std::thread` pinned to its core via `pthread_setaffinity_np`. A `boost::asio::io_context::work` per context keeps the loops alive until `Stop()`.
- `CServer` runs **its own** `io_context` on a separate `_acceptor_thread` — it is *not* part of the pool. The acceptor coroutine `StartAcceptLoop()` calls `pool->GetNextIOService()` to round-robin assign each new `CSession` to a pool io_context, then awaits `async_accept` on the new session's socket.
- Once a `CSession` is constructed against a pool io_context, **every operation on that session must run on that same executor**. This is why `Send()`, `Close()`, and `LogicSystem::PostMsgToQue` all wrap their bodies in `boost::asio::post(_socket.get_executor(), ...)`. The `_send_que` and `_b_close` flag are accessed without locks because they are only ever touched on the session's own thread.

When modifying session state or adding new entry points to `CSession`, preserve this invariant: cross-thread callers must `post` onto the session's executor; same-thread callers may touch state directly.

### Wire protocol

Frames are `[msg_id:2][length:2][body:N]` with `msg_id` and `length` in **network byte order**. `CSession::HandleRead` reads the 4-byte head into `_recv_head_node`, validates `msg_id` is in `(MSG_ID_MIN, MSG_ID_MAX)` and `length ≤ MAX_LENGTH` (2048), then reads the body and posts a `LogicNode` to `LogicSystem`. Constants live in `include/const.h`; new message types go in the `MSG_IDS` enum.

### Logic dispatch

`LogicSystem` (singleton) holds a `msg_id → callback` map populated by `RegisterCallBacks()`. `PostMsgToQue` does **not** queue — it immediately `post`s the callback onto the session's own executor. So message handlers execute on the same thread that did the read, which means a handler calling `session->Send(...)` is already on the right executor (the inner `post` in `Send` is still required because external callers may not be).

To add a new message type: add an entry to `MSG_IDS`, add a member function in `LogicSystem`, and bind it in `RegisterCallBacks()`.

### Send path

`CSession::Send` enqueues a `SendNode` and spawns `HandleWrite` **only when the queue was previously empty** — a single writer coroutine drains the queue until empty, then exits. Subsequent `Send` calls re-spawn it. `HandleWrite` retries `async_write` up to 3 times with backoff (`50ms × retry`) before calling `Close()`. Don't add a parallel writer; the empty-queue check is the concurrency guard.

### Lifetime

Sessions are kept alive by `_sessions` (keyed by UUID) on `CServer` and by `shared_from_this()` captures in spawned coroutines. `CServer::ClearSession` posts the erase onto the acceptor's executor to avoid contending with the accept loop. `Close()` is idempotent via `_b_close`.

## Repo notes

- `main_server.cpp` / `main_jsonclient.cpp` live at the repo root, not under `src/`. CMake only globs `src/*.cpp` for the library sources; entry-point files are listed explicitly.
- `plan.md` and `custom.md` are author-local design notes; they are not authoritative API documentation.
- `.vscode/c_cpp_properties.json` is the source of truth for IDE include paths.
