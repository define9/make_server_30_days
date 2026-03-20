# Day06: 非阻塞I/O

## 今日目标

在Day05基础上，深入理解非阻塞I/O，将server socket和accept都改为非阻塞模式，实现完整的非阻塞服务器。

## 核心改动

相比Day05的变化：
- **Server**：server socket设置`O_NONBLOCK`
- **Server**：`acceptAllConnections()`循环接受直到`EAGAIN`
- 确保多连接同时到达时不会遗漏

## 新学知识

### 1. 三次握手 vs accept

```
TCP三次握手：内核完成（操作系统协议栈）
accept()：从内核的"已完成连接队列"取一个出来
```

**accept不处理三次握手，那是内核的事。**

### 2. 核心目标：让accept不阻塞

**我们想要的是：循环accept直到队列空**

```
epoll通知1次 READ事件
  ↓
while (true) {
    accept() → 返回连接1
    accept() → 返回连接2
    accept() → 返回连接3
    accept() → ??? 
}
```

**问题是：最后一个accept会怎样？**

| server socket模式 | 最后一个accept的行为 |
|-------------------|---------------------|
| 阻塞模式 | 队列空时**永久等待**（程序卡住！） |
| O_NONBLOCK模式 | 队列空时**立即返回EAGAIN**（循环退出） |

**所以O_NONBLOCK的核心作用**：
- 不设置：循环到最后一个accept会**阻塞等新连接**
- 设置了：循环到最后一个accept会**返回EAGAIN，循环退出**

**一句话总结**：
> 设置O_NONBLOCK不是为了让accept快，而是为了让accept在队列空时不阻塞，能安全地循环退出。

### 3. accept的EAGAIN处理

非阻塞accept返回-1时，需要判断errno：

```c
int clientSocket = accept(serverSocket, ...);
if (clientSocket < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 没有更多连接，退出循环
        break;
    }
    if (errno == EINTR) {
        // 被信号中断，重试
        continue;
    }
    // 其他错误
    break;
}
```

### 3. 完整的非阻塞服务器

Day06实现完全非阻塞：

```
Server socket: O_NONBLOCK
  ↓
accept(): 循环直到EAGAIN
  ↓
Client socket: O_NONBLOCK (Connection构造时设置)
  ↓
read()/write(): 循环直到EAGAIN
  ↓
epoll ET: 只在状态变化时通知
```

### 4. 非阻塞I/O的优缺点

**优点**：
- 不会阻塞在单个操作上
- 可以同时处理多个连接
- 适合高并发场景

**缺点**：
- 编程复杂（需要循环处理）
- CPU空转（busy polling）
- 需要配合其他机制（如epoll）使用

### 5. O_NONBLOCK vs SOCK_NONBLOCK

两种设置非阻塞的方式：

```c
// 方式1：fcntl
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// 方式2：socket选项（创建时直接非阻塞）
int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
```

Day06使用方式1，因为server socket在Day01就已创建。

## 代码结构

```
day06/src/
├── main.cpp           # 程序入口 - 无变化
├── Server.h/cpp       # server socket O_NONBLOCK + 循环accept
├── EventLoop.h/cpp    # 事件循环 - 回调acceptAllConnections
├── Selector.h/cpp     # epoll封装 - 无变化
└── Connection.h/cpp   # 非阻塞I/O - 无变化
```

## 编译运行

```bash
cd day06
make
sudo ./server 8080

# 测试：多个客户端同时连接
nc localhost 8080 &
nc localhost 8080 &
nc localhost 8080 &
```

## 预期输出

```
[Server] Server listening on port 8080
[EventLoop] Started (ET mode)
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5)
[Server] Accepted client: 127.0.0.1:xxxxx (fd=6)
[Server] Accepted client: 127.0.0.1:xxxxx (fd=7)
[Server] Accepted 3 connections in one batch
```

## 下一步

Day07将学习**Reactor模式**，引入事件源和Handler注册机制。