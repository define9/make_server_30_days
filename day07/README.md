# Day07: Reactor模式基础

## 今日目标

在Day06基础上，引入Reactor模式，建立事件源和Handler注册机制。

## 核心改动

相比Day06的变化：
- **EventHandler接口**：定义handleRead/handleWrite/handleClose/interest抽象方法
- **Reactor类**：事件分发中心，管理所有Handler的注册和事件分发
- **Server实现EventHandler**：接受连接作为事件处理
- **Connection实现EventHandler**：处理读写作为事件处理

## 新学知识

### 1. Reactor模式核心组件

```
┌─────────────────────────────────────────────────────┐
│                     Reactor                         │
│  ┌─────────────────────────────────────────────┐   │
│  │           事件分发中心                        │   │
│  │  1. 持有Selector进行I/O多路复用               │   │
│  │  2. 管理所有注册的EventHandler              │   │
│  │  3. 事件循环：等待 → 分发 → 处理              │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
         ↑ registerHandler()         ↑ registerHandler()
         │                            │
    ┌────┴────┐                  ┌────┴────┐
    │ Server  │                  │Connection│
    │(Handler)│                  │(Handler) │
    └─────────┘                  └──────────┘
```

### 2. EventHandler接口

```cpp
class EventHandler {
public:
    virtual int fd() const = 0;
    virtual bool handleRead() = 0;
    virtual bool handleWrite() = 0;
    virtual void handleClose() = 0;
    virtual EventType interest() const = 0;
    virtual bool hasDataToSend() const = 0;
};
```

### 3. 事件流

```
[发生I/O事件]
     ↓
[Reactor.wait()] 返回就绪的fd
     ↓
[根据fd找到Handler]
     ↓
[调用Handler->handleXxx()]
     ↓
[根据返回值决定是否移除Handler]
```

### 4. 为什么需要Reactor模式

**解耦**：事件源和事件处理逻辑分离
- Server只关心accept
- Connection只关心读写
- Reactor统一调度

**统一**：所有Handler实现同一接口
- 事件分发逻辑一致
- 方便扩展新Handler

## 代码结构

```
day07/src/
├── main.cpp           # 程序入口 - Reactor驱动
├── EventHandler.h     # 事件处理器抽象接口
├── Reactor.h/cpp      # 事件分发中心
├── Server.h/cpp       # 服务器 - 实现EventHandler
├── Connection.h/cpp   # 连接 - 实现EventHandler
└── Selector.h/cpp    # epoll封装
```

## 编译运行

```bash
cd day07
make
sudo ./server 8080

# 测试
nc localhost 8080 &
nc localhost 8080 &
```

## 预期输出

```
[Server] Server listening on port 8080
[Reactor] Started
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5)
[Reactor] Started
```

## 下一步

Day08将学习完整Reactor实现，整合多路复用。
