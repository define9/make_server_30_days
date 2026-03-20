# Day03: poll多路复用

## 今日目标

在Day02基础上，将`select()`替换为`poll()`，解决FD_SETSIZE限制问题。

## 核心改动

相比Day02的变化：
- **Selector类重写**：使用`poll()`代替`select()`
- **pollfd数组**：使用`struct pollfd`动态数组，无fd数量限制
- **事件类型**：使用`POLLIN`/`POLLOUT`/`POLLERR`等常量

## 新学知识

### 1. poll()原理

```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

- `fds`: pollfd数组指针
- `nfds`: 数组中元素个数
- `timeout`: 超时时间(毫秒)

**struct pollfd**:
```c
struct pollfd {
    int fd;        // 文件描述符
    short events;  // 监控的事件(POLLIN, POLLOUT, POLLERR等)
    short revents; // 返回的事件(由内核填充)
};
```

### 2. poll vs select

| 特性 | select | poll |
|------|--------|------|
| fd数量限制 | FD_SETSIZE(通常1024) | 无限制(受系统限制) |
| 数据结构 | fd_set(固定大小位图) | pollfd数组(动态) |
| 事件定义 | 使用位标志 | 使用事件常量 |
| 监控事件 | read/write/error | IN/OUT/ERR/HUP/NVAL等 |
| 重置 | 每次调用需要重新设置 | 每次调用需要重新设置 |
| 效率 | O(n) | O(n) |

### 3. poll的事件类型

```c
// 监控事件(events)
POLLIN     // 可读
POLLOUT    // 可写
POLLPRI    // 紧急数据可读

// 返回事件(revents)
POLLERR    // 错误
POLLHUP    // 挂断
POLLNVAL   // fd未打开
```

### 4. poll的优点

- **无fd数量限制**：可以监控任意数量的fd（受系统`/proc/sys/fs/file-max`限制）
- **更清晰的API**：pollfd结构更直观
- **跨平台**：Linux、macOS都支持（Windows不支持poll）

### 5. poll的缺点

- 仍然需要遍历所有fd检查revents
- 每次调用仍需从用户态拷贝到内核态
- 大规模fd下效率仍然不高（见Day04 epoll）

## 代码结构

```
day03/src/
├── main.cpp           # 程序入口
├── Server.h/cpp      # 服务器(接受连接) - 无变化
├── EventLoop.h/cpp   # 事件循环 - poll封装 - 有小变化
├── Selector.h/cpp    # select封装 -> poll封装 - 完全重写
├── Connection.h/cpp # 客户端连接 - 无变化
└── Buffer.h/cpp     # 读写缓冲区(可选)
```

## 编译运行

```bash
cd day03
make
sudo ./server 8080

# 测试多个客户端(另开多个终端)
nc localhost 8080
nc localhost 8080
nc localhost 8080
```

## 预期输出

```
[Server] Server listening on port 8080
[EventLoop] Started
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5)
[EventLoop] Added connection fd=5
[EventLoop] Server socket ready for accept
[Server] Accepted client: 127.0.0.1:xxxxx (fd=6)
[EventLoop] Added connection fd=6
[Connection] Read 6 bytes from 127.0.0.1:xxxxx: Hello
[Connection] Wrote 6 bytes to 127.0.0.1:xxxxx
```

## 下一步

Day04将学习`epoll`，解决poll的O(n)遍历问题，实现O(1)事件通知。
