# ThreadPoolServer

基于 Boost.Asio + C++20 无栈协程的高性能 TCP 服务器。

## 特性

- **C++20 协程**：使用 Boost.Asio 无栈协程（stackless coroutine），代码更简洁
- **多线程池**：利用系统多核优势，提升并发性能
- **消息分片**：支持 TCP 粘包处理，自定义协议解析
- **QPS 统计**：内置请求数、响应数、QPS、回包完整率统计

## 目录结构

```
ThreadPoolServer/
├── include/                 # 头文件
│   ├── AsioThreadPool.h    # 线程池
│   ├── CServer.h           # 服务器
│   ├── CSession.h          # 客户端会话
│   ├── const.h             # 常量定义
│   ├── LogicSystem.h       # 逻辑系统
│   ├── MsgNode.h           # 消息节点
│   └── Singleton.h         # 单例模板
├── src/                    # 源文件
│   ├── AsioThreadPool.cpp
│   ├── CServer.cpp
│   ├── CSession.cpp
│   ├── LogicSystem.cpp
│   └── MsgNode.cpp
├── main_server.cpp         # 服务器入口
├── main_jsonclient.cpp     # 测试客户端
├── CMakeLists.txt         # CMake 构建配置
└── build/                 # 构建目录
```

## 消息协议

消息格式：`[2字节 msg_id][2字节 length][数据]`

```
+--------+--------+------------+
| msg_id | length |    data    |
+--------+--------+------------+
   2B      2B         N B
```

## 编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
make
```

## 运行

```bash
# 启动服务器
./ThreadPoolServer

# 启动测试客户端
./main_jsonclient
```

## 服务器输出示例

```
Server start success, listen on port : 10086
[QPS] 1000 [Total] 5000 [ResponseRate] 100%
[QPS] 1200 [Total] 6200 [ResponseRate] 100%
========== Server Statistics ==========
Total Requests:   6500
Total Responses:  6500
Response Rate:    100%
Average QPS:      1083
Uptime:           6s
=========================================
```

## 依赖

- Boost (>= 1.70)
- JSONCpp
- C++20 兼容编译器 (GCC 13+)
