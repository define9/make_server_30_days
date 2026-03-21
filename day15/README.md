# Day15: 信号处理

## 今日目标

在Day14的配置系统基础上，添加完整的信号处理机制：
- SignalHandler封装
- SIGINT/SIGTERM/SIGHUP/SIGPIPE处理
- 优雅退出流程
- 回调函数注册机制

## 核心改动

相比Day14的变化：
- **新增SignalHandler类**：统一的信号处理管理
- **使用sigaction替代signal**：更可靠、可移植
- **回调函数机制**：支持注册关闭/重载回调
- **SIGPIPE忽略**：防止向已关闭socket写数据导致崩溃
- **优雅退出流程**：先停止接受新连接，再关闭现有连接

关于信号处理设计：
> 信号处理是服务器开发中的重要组成部分。我们使用sigaction代替signal，因为它的行为更确定、更可移植。SignalHandler作为单例，提供回调注册机制，使得信号处理逻辑与业务逻辑分离。

## 信号处理架构

```
              ┌─────────────────────────────────────┐
              │            SignalHandler             │
              │  ┌─────────────────────────────────┐  │
              │  │      Signal-to-Action Map       │  │
              │  │  SIGINT  → shutdown callback    │  │
              │  │  SIGTERM → shutdown callback    │  │
              │  │  SIGHUP  → reload callback     │  │
              │  │  SIGPIPE → ignored             │  │
              │  └─────────────────────────────────┘  │
              └─────────────────────────────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
 ┌─────────────┐      ┌─────────────┐      ┌─────────────┐
 │ onShutdown  │      │  onReload   │      │ sigaction   │
 │ callbacks   │      │  callbacks  │      │  (internal) │
 └─────────────┘      └─────────────┘      └─────────────┘
```

## 信号说明

| 信号 | 默认行为 | 我们的处理 | 用途 |
|------|----------|------------|------|
| SIGINT | 进程终止 | shutdown | Ctrl+C 终止 |
| SIGTERM | 进程终止 | shutdown | kill命令优雅终止 |
| SIGHUP | 进程终止 | reload | 配置重载 |
| SIGPIPE | 进程终止 | 忽略 | 防止socket写崩溃 |

## 为什么需要SignalHandler

### 标准signal()的问题

1. **行为不一致**：System V和BSD的signal()行为不同
2. **每次调用重置**：需要每次调用signal()重新注册
3. **处理函数受限**：信号处理函数中能调用的函数有限

### 我们的解决方案

使用sigaction：
- 行为确定（POSIX标准）
- 自动重启系统调用（SA_RESTART）
- 可保存旧处理函数

## SignalHandler 接口

```cpp
class SignalHandler {
public:
    static SignalHandler& instance();

    void start();                                    // 启动信号处理
    void wait();                                     // 阻塞等待信号
    void onShutdown(std::function<void()> callback);  // 注册关闭回调
    void onReload(std::function<void()> callback);   // 注册重载回调
    bool isShuttingDown() const;                    // 检查关闭状态
    void signalShutdown();                          // 发送关闭信号
};
```

## 使用方式

```cpp
int main() {
    ReactorPool reactorPool(port, workers);

    // 注册信号回调
    SignalHandler::instance().onShutdown([&]() {
        LOG_INFO("Shutting down...");
        reactorPool.stop();
    });

    SignalHandler::instance().onReload([]() {
        LOG_INFO("Reloading config...");
    });

    SignalHandler::instance().start();
    reactorPool.start();

    // 唯一的阻塞点：等待 reactor 退出
    // epoll_wait() 会被信号中断（SA_RESTART），然后触发 shutdown callback
    reactorPool.waitForExit();

    return 0;
}
```

> **注意**：`signalHandler.wait()` 是冗余的——因为 `waitForExit()` 内部调用 `m_mainReactor->loop()`，
> 其中 `epoll_wait()` 会被信号中断并重启，中断后触发 shutdown callback 使 reactor 停止，
> 最终整个程序结束。只需要一个阻塞点。

## 信号与线程安全

信号处理中的线程安全考虑：

1. **信号处理函数应尽量简单**：只设置标志位，不做复杂操作
2. **回调在主线程执行**：我们的设计确保回调在注册线程调用
3. **原子操作**：使用std::atomic保证标志位安全

## 代码结构

```
day15/src/
├── main.cpp                    # 程序入口
├── signal/                     # 信号处理领域（新增）
│   ├── SignalHandler.h       # 信号处理器
│   └── SignalHandler.cpp     # 信号处理器实现
├── config/                     # 配置领域（继承自day14）
├── net/                       # 网络领域
├── reactor/                   # Reactor领域
├── thread/                    # 线程池领域
├── timer/                     # 定时器领域
└── log/                       # 日志领域
```

## 编译运行

```bash
cd day15
make

# 运行服务器
sudo ./server

# 测试优雅退出
kill -SIGTERM $(pidof server)

# 测试配置重载
kill -SIGHUP $(pidof server)

# Ctrl+C退出
./server
# 按Ctrl+C
```

## 预期输出

```
========================================
  Day15: Signal Handling
========================================
Config file: server.conf
Port: 8080
...
Signal handler started
Main Reactor started
Worker Reactor 0 started
...

# 按Ctrl+C或kill -SIGTERM
Received SIGINT/SIGTERM, initiating shutdown...
Signal shutdown callback triggered
Signal shutdown callback triggered end
Main Reactor stopped
Server stopped gracefully
```

## 下一步

Day16将学习地址复用：SO_REUSEADDR、快速重启。
