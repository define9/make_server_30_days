# Day10: 线程池实现 - ThreadPool

## 今日目标

在Day09的Reactor线程池基础上，学习通用的线程池实现：
- 任务队列（TaskQueue）
- Worker线程管理
- 线程池（ThreadPool）

## 核心改动

相比Day09的变化：
- **新增TaskQueue类**：线程安全的任务队列，支持阻塞pop
- **新增ThreadPool类**：独立的线程池，可用于任意任务
  - 我们只是用作了demo，ReactorPool里掌管的 子Reactor 线程没有使用线程池，因为这些子线程都是一直loop不会结束的,
  而线程池适合一些小的耗时的任务
- **按领域分类代码**：net/、reactor/、thread/ 三个目录，使其看起来并不臃肿

## 设计思想

### 1. ThreadPool vs Reactor Workers

```
┌─────────────────────────────────────────────────────────┐
│                     ThreadPool (通用)                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                 │
│  │ Worker  │  │ Worker  │  │ Worker  │  ← 任务循环   │
│  │ [task]  │  │ [task]  │  │ [task]  │    取-执行-释放 │
│  │ [task]  │  │ [task]  │  │ [task]  │                 │
│  └─────────┘  └─────────┘  └─────────┘                 │
│         适用于: 日志写入、DB操作、文件处理等              │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                 Reactor Workers (专用)                    │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                 │
│  │ Worker  │  │ Worker  │  │ Worker  │  ← 永久循环   │
│  │[loop()] │  │[loop()] │  │[loop()] │    永不释放    │
│  └─────────┘  └─────────┘  └─────────┘                 │
│         适用于: 事件循环 (网络IO、定时器等)               │
└─────────────────────────────────────────────────────────┘
```

### 2. 为什么不共用线程池？

| 特性 | ThreadPool | Reactor Workers |
|------|------------|-----------------|
| 生命周期 | 任务完成即释放 | 永久运行至服务器关闭 |
| 任务类型 | 短期异步任务 | 长期事件循环 |
| 复用性 | 线程可复用处理多任务 | 线程专属、不释放 |
| 退出方式 | 所有任务完成后退出 | 收到stop信号后退出 |

**结论**：Reactor Workers 是"常驻进程"，不适合放入通用线程池。
ThreadPool 的真正价值在于处理**耗时阻塞操作**（如HTTP服务器的请求处理、数据库查询等）。

## 代码结构

```
day10/src/
├── main.cpp                    # 程序入口
├── net/                       # 网络领域
│   ├── Selector.h/cpp         # epoll封装
│   ├── Connection.h/cpp       # 客户端连接
│   └── Server.h/cpp           # TCP服务器
├── reactor/                   # Reactor领域
│   ├── EventHandler.h         # 事件处理器接口
│   ├── Reactor.h/cpp          # 事件分发中心
│   └── ReactorPool.h/cpp      # Reactor线程池
└── thread/                    # 线程池领域
    ├── TaskQueue.h/cpp        # 任务队列
    └── ThreadPool.h/cpp       # 线程池
```

## 新学知识

### 1. 生产者-消费者模式

```
┌─────────────┐    任务     ┌─────────────┐
│  生产者     │ ────────▶  │  任务队列    │
│  (主线程)   │            │  (线程安全)  │
└─────────────┘            └──────┬──────┘
                                 │
                    ┌────────────┼────────────┐
                    ▼            ▼            ▼
              ┌─────────┐  ┌─────────┐  ┌─────────┐
              │ Worker  │  │ Worker  │  │ Worker  │
              │  线程1  │  │  线程2  │  │  线程3  │
              └─────────┘  └─────────┘  └─────────┘
```

### 2. TaskQueue线程安全机制

- std::mutex：保护队列操作
- std::condition_variable：阻塞等待任务
- std::atomic：任务计数

### 3. ThreadPool API

```cpp
ThreadPool pool(4);     // 4个工作线程
pool.start();          // 启动
pool.submit([](){      // 提交任务
    // 执行任务
});
pool.stop();            // 优雅停止
```

## 编译运行

```bash
cd day10
make
sudo ./server 8080

# 测试
nc localhost 8080 &
echo "hello" | nc localhost 8080
```

## 预期输出

```
========================================
  Day10: ThreadPool Demo (演示用)
========================================
[ThreadPool] Started 4 worker threads
[Main] Submitting demo tasks to ThreadPool...
[Task] Worker 0 executing...
[Task] Worker 1 executing...
[Task] Worker 2 executing...
[Main] Demo tasks completed: 3 executed

========================================
  ReactorPool (Network I/O) - 真正工作的线程
========================================
[Server] Server listening on port 8080
[Reactor] Main Reactor started
[Reactor] Worker Reactor 0 started
[Reactor] Worker Reactor 1 started
[Reactor] Worker Reactor 2 started
[Reactor] Worker Reactor 3 started
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5) -> Worker 0
[Connection] Worker 0 read 5 bytes from 127.0.0.1:xxxxx: hello
```

## 下一步

Day11将学习定时器实现：最小堆、时间轮、过期连接处理。
