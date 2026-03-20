# Day05: epoll边缘触发(ET)

## 今日目标

在Day04基础上，将epoll从LT(水平触发)改为ET(边缘触发)模式，配合非阻塞I/O实现高效事件处理。

## 核心改动

相比Day04的变化：
- **Selector**：添加`EPOLLET`标志
- **Connection**：使用`O_NONBLOCK`非阻塞I/O，循环读写直到`EAGAIN`
- **EventLoop**：动态注册WRITE事件（有数据时）

## 新学知识

### 1. LT vs ET 原理

```
LT (水平触发):
时间 ──────────────────────────────────>
       │                              │
       ▼                              ▼
    可读                             可读
    通知！                            通知！

       ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━▶
       期间任何时候都可以读取，epoll_wait都会返回


ET (边缘触发):
时间 ──────────────────────────────────>
       │                              │
       ▼                              ▼
     变为                           变为
     可读                            可读
     通知！                           通知！

       ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━▶
       只在状态变化时通知一次，必须一次性处理完
```

### 2. ET模式的核心要求

**ET必须配合非阻塞I/O**：

```c
// 1. 设置非阻塞
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// 2. 循环读取直到EAGAIN
while (true) {
    n = read(fd, buf, sizeof(buf));
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;  // 数据处理完毕
        }
        // 错误处理
    }
}
```

### 3. EAGAIN vs EWOULDBLOCK

- `EAGAIN`：资源暂时不可用（Linux）
- `EWOULDBLOCK`：同EAGAIN（BSD）
- 在非阻塞socket上，这两个errno是相同的

### 4. 为什么ET需要动态注册WRITE事件？

LT模式下：
- 只要写缓冲区不满，WRITE一直触发
- 浪费CPU

ET模式下：
- WRITE只在"变为可写"时触发一次
- 如果没写完（缓冲区满），需要等下次"变为可写"
- 所以写完数据后，要取消WRITE监听
- 有新数据要发送时，要注册WRITE监听

### 5. ET的优点

- **更少唤醒**：只在状态变化时通知
- **更高效率**：避免重复处理已就绪的fd
- **适合大并发**：减少无效的系统调用

### 6. ET的缺点

- **编程复杂**：必须循环处理所有I/O
- **容易漏事件**：如果没处理完数据，事件不会重复通知
- **调试困难**：问题不易复现

### 7. 惊群效应(Thundering Herd)

当多个进程/线程等待同一个socket时，如果来一个新连接，所有等待者都会被唤醒，但只有1个能处理。

Day05不处理惊群，后续会学到。

## 代码结构

```
day05/src/
├── main.cpp           # 程序入口 - 无变化
├── Server.h/cpp      # 服务器 - 无变化
├── EventLoop.h/cpp   # 事件循环 - 动态注册WRITE事件
├── Selector.h/cpp    # epoll ET标志 - addFd/modifyFd加EPOLLET
└── Connection.h/cpp  # 非阻塞I/O - O_NONBLOCK + 循环读写
```

## 编译运行

```bash
cd day05
make
sudo ./server 8080

# 测试
nc localhost 8080
hello  # 输入
hello  # 输出（echo）
```

## 预期输出

```
[Server] Server listening on port 8080
[EventLoop] Started (ET mode)
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5)
[EventLoop] Added connection fd=5
[Connection] Read total 6 bytes from 127.0.0.1:xxxxx: hello
[Connection] Wrote 6 bytes to 127.0.0.1:xxxxx
```

## 下一步

Day06将学习**非阻塞I/O**，深入理解O_NONBLOCK、connect/accept的非阻塞行为。
