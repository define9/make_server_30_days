# Day04: epoll基础

## 今日目标

在Day03基础上，将`poll()`替换为`epoll()`，实现O(1)复杂度的事件通知。

## 核心改动

相比Day03的变化：
- **Selector类重写**：使用`epoll()`代替`poll()`
- **epoll_create**：创建epoll实例（内核中的红黑树）
- **epoll_ctl**：添加/删除/修改监控的fd
- **epoll_wait**：等待事件发生
- **LT模式**：默认的水平触发模式

## 新学知识

### 1. epoll原理

epoll是Linux特有的I/O多路复用机制，相比poll和select有显著性能优势：

```
用户态                    内核态
┌─────────┐            ┌─────────────┐
│  应用    │ ──addFd──> │   红黑树     │
│         │ ──wait──>  │  (fd管理)    │
│         │ <─ready─── │   就绪列表    │
└─────────┘            └─────────────┘
```

**核心优势**：
- 无需遍历所有fd，O(1)复杂度
- 内核使用红黑树管理fd，效率高
- 只返回已就绪的fd

### 2. epoll三个系统调用

```c
// 1. 创建epoll实例
int epoll_create1(int flags);  // flags=0或EPOLL_CLOEXEC

// 2. 管理监控的fd
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
// op: EPOLL_CTL_ADD / EPOLL_CTL_DEL / EPOLL_CTL_MOD

// 3. 等待事件
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

### 3. struct epoll_event

```c
struct epoll_event {
    uint32_t events;    // 监控的事件(EPOLLIN, EPOLLOUT, EPOLLERR等)
    epoll_data_t data; // 用户数据，通常是fd
};

typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
```

### 4. epoll事件类型

```c
// 监控事件
EPOLLIN   // 可读
EPOLLOUT  // 可写
EPOLLERR  // 错误（自动监控）
EPOLLHUP  // 挂断（自动监控）

// 工作模式
EPOLLLT   // 水平触发（默认）
EPOLLET   // 边缘触发
```

### 5. 水平触发(LT) vs 边缘触发(ET)

**水平触发(LT)** - 默认模式：
- 只要条件满足，就一直通知
- 漏读数据会自动继续提醒
- 编程简单，类似poll

**边缘触发(ET)**：
- 状态变化时只通知一次
- 需要一次性读完所有数据
- 编程复杂，但效率更高

### 6. epoll的优点

- **O(1)复杂度**：只返回已就绪的fd
- **无fd数量限制**：受系统`/proc/sys/fs/file-max`限制
- **高效管理**：内核使用红黑树
- **支持边缘触发**：可进一步提升性能

### 7. epoll的缺点

- **Linux特有**：不能跨平台
- **编程复杂度**：比poll稍复杂

## 代码结构

```
day04/src/
├── main.cpp           # 程序入口
├── Server.h/cpp      # 服务器 - 无变化
├── EventLoop.h/cpp   # 事件循环 - 小变化
├── Selector.h/cpp    # poll封装 -> epoll封装 - 完全重写
└── Connection.h/cpp  # 客户端连接 - 无变化
```

## 编译运行

```bash
cd day04
make
sudo ./server 8080

# 测试多个客户端
nc localhost 8080
nc localhost 8080
```

## 预期输出

```
[Server] Server listening on port 8080
[EventLoop] Started
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5)
[EventLoop] Added connection fd=5
[Connection] Read 6 bytes from 127.0.0.1:xxxxx: Hello
[Connection] Wrote 6 bytes to 127.0.0.1:xxxxx
```

## 下一步

Day05将学习`epoll边缘触发(ET)`模式，进一步提升性能。
