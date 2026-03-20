# Day02: select多路复用

## 今日目标

在Day01基础上，使用`select()`实现I/O多路复用，同时处理多个客户端。

## 核心改动

相比Day01的变化：
- **新增Selector类**：封装fd_set操作和select调用
- **新增Connection类**：封装单个客户端连接的状态和数据缓冲区
- **新增EventLoop类**：事件循环，分发I/O事件
- **Server简化**：服务器专注于接受连接，EventLoop处理所有I/O

## 新学知识

### 1. I/O多路复用 (Multiplexing)

**问题**：阻塞I/O一次只能处理一个客户端，其他客户端必须等待。

**解决方案**：使用I/O多路复用，一个线程同时监控多个文件描述符。

### 2. select()原理

```c
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
```

- `nfds`: 监控的最大fd + 1
- `readfds`: 监控可读事件的fd集合
- `writefds`: 监控可写事件的fd集合
- `exceptfds`: 监控异常事件的fd集合
- `timeout`: 超时时间

**工作流程**：
```
1. 设置要监控的fd和事件类型到fd_set
2. 调用select()阻塞等待
3. select()返回时，内核修改fd_set，只保留有事件的fd
4. 遍历fd，检测哪些fd有事件
5. 处理事件
6. 回到步骤1
```

### 3. fd_set操作

```c
FD_ZERO(&set);    // 清空集合
FD_SET(fd, &set); // 添加fd到集合
FD_CLR(fd, &set); // 从集合移除fd
FD_ISSET(fd, &set); // 检测fd是否在集合中
```

### 4. FD_SETSIZE限制

- `select()`使用固定大小的fd_set（默认1024）
- 最多同时监控1024个fd
- 现代系统可调整，但有上限

### 5. 面向对象设计改进

**Day01设计**：
```
Server -> ClientHandler (1:1, 阻塞)
```

**Day02设计**：
```
EventLoop (1) -> Selector (1) -> Connection (N)
                     ^
                     |
Server (接受连接) ----+
```

- EventLoop是中央调度器
- Selector封装select系统调用
- Connection封装每个客户端的状态

## 代码结构

```
day02/src/
├── main.cpp           # 程序入口
├── Server.h/cpp      # 服务器(接受连接)
├── EventLoop.h/cpp   # 事件循环(核心调度)
├── Selector.h/cpp    # select封装
├── Connection.h/cpp  # 客户端连接
└── Buffer.h/cpp      # 读写缓冲区(可选)
```

## 编译运行

```bash
cd day02
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
[Server] Accepted client fd=5
[Selector] Client connected: fd=5
[Selector] Client connected: fd=6
[Selector] Client fd=5 ready to read
[Selector] Client fd=6 ready to read
[Selector] Client fd=5 read: Hello
[Selector] Client fd=5 echo back: Hello
[Selector] Client fd=6 read: World
[Selector] Client fd=6 echo back: World
```

## select的优缺点

**优点**：
- 跨平台（Windows/Linux/macOS都支持）
- API简单
- 适合少量连接

**缺点**：
- FD_SETSIZE限制（通常1024）
- 每次调用需要从用户态拷贝fd_set到内核态
- 每次返回需要遍历所有fd检查
- 不支持动态修改监控集合

## 下一步

Day03将学习`poll()`，解决FD_SETSIZE限制问题。
