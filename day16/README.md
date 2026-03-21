# Day16: 网络选项优化

## 今日目标

在Day15的信号处理基础上，优化网络socket选项：
- SO_REUSEADDR 地址复用
- SO_REUSEPORT 端口复用
- TCP_NODELAY 禁用Nagle算法
- SO_KEEPALIVE TCP保活
- SocketOptions统一管理类

## 核心改动

相比Day15的变化：
- **新增SocketOptions类**：统一的socket选项管理
- **TCP_NODELAY**：对每个连接启用，降低延迟
- **SO_REUSEPORT支持**：Linux 3.9+多进程共享监听端口
- **配置化选项**：ServerOptions和ClientOptions结构体

## 为什么需要Socket选项优化

### SO_REUSEADDR - 地址复用

**问题**：服务器关闭后，端口可能被内核保留（TIME_WAIT状态），导致立即重启失败。

**解决**：启用SO_REUSEADDR，允许bind已关闭socket的地址。

```cpp
int opt = 1;
setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### SO_REUSEPORT - 端口复用

**问题**：多进程模式下，多个进程无法共享同一监听端口。

**解决**：启用SO_REUSEPORT，内核允许负载均衡到多个socket。

```cpp
int opt = 1;
setsockopt(sockFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
```

> 注意：需要Linux 3.9+内核支持

### TCP_NODELAY - 禁用Nagle算法

**问题**：Nagle算法为减少小数据包会增加延迟（最多200ms）。

**解决**：对小数据包敏感的交互式应用，禁用Nagle。

```cpp
int opt = 1;
setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
```

### SO_KEEPALIVE - TCP保活

**问题**：TCP连接可能永久处于半开状态，浪费资源。

**解决**：启用keepalive，检测空闲连接。

```cpp
int opt = 1;
setsockopt(sockFd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
```

## Socket选项架构

```
┌─────────────────────────────────────────────────────────┐
│                    SocketOptions                         │
├─────────────────────────────────────────────────────────┤
│  ServerOptions          │       ClientOptions           │
│  ├─ reuseAddr           │       ├─ tcpNoDelay           │
│  ├─ reusePort           │       ├─ keepAlive            │
│  ├─ sendBuffer          │       ├─ sendBuffer           │
│  ├─ recvBuffer          │       ├─ recvBuffer           │
│  └─ nonBlock            │       └─ lingerTimeout        │
└─────────────────────────────────────────────────────────┘
                          │
         ┌────────────────┼────────────────┐
         ▼                ▼                ▼
  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
  │  Server     │ │  Connection  │ │   Other     │
  │  (listen)   │ │  (per-cxn)  │ │   sockets   │
  └─────────────┘ └─────────────┘ └─────────────┘
```

## 选项说明

| 选项 | 作用域 | 默认值 | 说明 |
|------|--------|--------|------|
| SO_REUSEADDR | Server | 启用 | 允许bindTIME_WAIT地址 |
| SO_REUSEPORT | Server | 禁用 | 多进程共享监听端口 |
| TCP_NODELAY | Client | 启用 | 禁用Nagle，降低延迟 |
| SO_KEEPALIVE | Client | 禁用 | TCP保活检测 |
| SO_LINGER | Client | 0超时 | 优雅关闭等待 |

## SocketOptions接口

```cpp
class SocketOptions {
public:
    struct ServerOptions {
        bool reuseAddr = true;
        bool reusePort = false;
        int sendBuffer = 0;
        int recvBuffer = 0;
        bool nonBlock = true;
    };

    struct ClientOptions {
        bool tcpNoDelay = true;
        bool keepAlive = false;
        int sendBuffer = 0;
        int recvBuffer = 0;
        bool nonBlock = true;
        int lingerTimeout = 0;
    };

    static bool applyServerOptions(int sockFd, const ServerOptions& options);
    static bool applyClientOptions(int sockFd, const ClientOptions& options);
};
```

## 使用方式

```cpp
// 服务器socket选项
SocketOptions::ServerOptions serverOpts;
serverOpts.reuseAddr = true;
serverOpts.reusePort = true;  // Linux 3.9+
SocketOptions::applyServerOptions(listenFd, serverOpts);

// 客户端连接选项
SocketOptions::ClientOptions clientOpts;
clientOpts.tcpNoDelay = true;   // 低延迟
clientOpts.keepAlive = true;    // 长连接保活
SocketOptions::applyClientOptions(connFd, clientOpts);
```

## 代码结构

```
day16/src/
├── main.cpp                    # 程序入口
├── signal/                     # 信号处理领域（继承自day15）
├── socket/                     # Socket选项领域（新增）
│   └── SocketOptions.h      # Socket选项配置
├── config/                     # 配置领域
├── net/                       # 网络领域
│   ├── Server.cpp           # 使用SocketOptions
│   └── Connection.cpp       # 使用TCP_NODELAY
├── reactor/                   # Reactor领域
├── thread/                   # 线程池领域
├── timer/                    # 定时器领域
└── log/                      # 日志领域
```

## 编译运行

```bash
cd day16
make

# 运行服务器
sudo ./server

# 测试快速重启（之前会失败）
./server &
sleep 1
kill %1
./server  # 应该立即启动成功
```

## 预期输出

```
========================================
  Day16: Network Options
========================================
Config file: server.conf
Port: 8080
...
Server listening on port 8080
Server socket options applied: REUSEADDR=on, NONBLOCK=on
...
```

## 下一步

Day17将开始HTTP协议解析：请求/响应格式、状态码、Header解析。