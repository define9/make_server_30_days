# Day09: 多线程Reactor - One Loop per Thread

## 今日目标

在Day08基础上，引入多线程Reactor模式，实现主Reactor接受连接、子Reactor处理连接的架构。

## 核心改动

相比Day08的变化：
- **主Reactor**：只在主线程运行，负责accept新连接
- **子Reactor池**：多个Worker线程，每个拥有独立的Reactor
- **负载均衡**：轮询分发连接到不同的子Reactor
- **线程安全**：使用mutex保护共享状态（连接计数）

## 新学知识

### 1. One Loop per Thread架构

```
                    ┌─────────────────────────────────────────┐
                    │              Main Reactor               │
                    │  ┌───────────────────────────────────┐  │
                    │  │     负责accept() + 分发连接        │  │
                    │  └───────────────────────────────────┘  │
                    └──────────────────┬──────────────────────┘
                                       │
                   ┌───────────────────┼───────────────────┐
                   │                   │                   │
                   ▼                   ▼                   ▼
          ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
          │  Reactor 1  │     │  Reactor 2  │     │  Reactor 3  │
          │  (Worker 1) │     │  (Worker 2) │     │  (Worker 3) │
          │  处理连接    │     │  处理连接    │     │  处理连接    │
          └─────────────┘     └─────────────┘     └─────────────┘
```

### 2. 线程池设计

```cpp
class ReactorPool {
    Reactor* m_mainReactor;      // 主Reactor（仅accept）
    std::vector<Reactor*> m_workers;  // 工作Reactor池
    std::vector<std::thread> m_threads;   // 工作线程
    int m_nextReactor;  // 轮询索引
    std::mutex m_mutex;  // 保护m_nextReactor
};
```

### 3. 负载均衡策略

采用Round-Robin（轮询）策略：
- 新连接到来时，依次分配给不同的Worker Reactor
- 简单高效，避免了锁竞争
- 需要mutex保护m_nextReactor计数器

### 4. 线程安全考虑

多线程Reactor的挑战：
- **共享状态**：总连接数、活跃连接数
- **竞争条件**：多个Worker可能同时更新计数
- **解决方案**：使用mutex保护共享数据

## 代码结构

```
day09/src/
├── main.cpp           # 程序入口 - 启动ReactorPool
├── EventHandler.h     # 事件处理器抽象接口
├── Reactor.h/cpp      # 事件分发中心 + 优雅关闭
├── Server.h/cpp       # 服务器 - 实现EventHandler
├── Connection.h/cpp  # 连接 - 实现EventHandler
├── Selector.h/cpp    # epoll封装
└── ReactorPool.h/cpp  # Reactor线程池
```

## 编译运行

```bash
cd day09
make
sudo ./server 8080

# 测试
nc localhost 8080 &
echo "hello" | nc localhost 8080
# Ctrl+C 优雅退出
```

## 预期输出

```
[Server] Server listening on port 8080
[Reactor] Main Reactor started
[Reactor] Worker Reactor 0 started
[Reactor] Worker Reactor 1 started
[Reactor] Worker Reactor 2 started
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5) -> Worker 0
[Connection] Read total 5 bytes from 127.0.0.1:xxxxx: hello
[Reactor] Closing all handlers...
[Reactor] Worker Reactor 0 stopped
[Reactor] Worker Reactor 1 stopped
[Reactor] Worker Reactor 2 stopped
[Reactor] Main Reactor stopped
========== Server Statistics ==========
Total connections handled: 1
======================================
Server stopped
```

## 下一步

Day10将学习线程池实现：任务队列、Worker线程管理。