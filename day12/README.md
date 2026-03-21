# Day12: 连接超时管理

## 今日目标

在Day11的定时器基础上，实现连接超时管理：
- 空闲连接检测
- 超时断开
- 资源回收

## 核心改动

相比Day11的变化：
- **新增ConnectionManager**：统一管理所有连接的定时器
- **Reactor集成ConnectionManager**：事件循环支持超时检查
- **连接生命周期管理**：注册→活动→超时检测→断开→回收

## 连接超时管理架构

```
     ConnectionManager
          │
          ├── 管理所有 Connection 对象
          │
          └── TimerManager ──定时检查──> 检查连接空闲时间
                                       │
                                       ▼
                                  断开超时连接
```

### 工作流程

1. **连接建立**：Server接受连接 → Reactor.registerHandler()
2. **注册定时器**：ConnectionManager.addConnection() 添加30秒超时定时器
3. **活动检测**：每次handleRead/handleWrite成功 → updateActivity() 重置定时器
4. **超时检测**：定时器tick → handleTimeout() → 关闭连接
5. **资源回收**：removeHandler() → delete Connection

### 关键设计

- **O(1)定时器重置**：每次IO活动时，删除旧定时器、添加新定时器
- **Timer Wheel优化**：使用时间轮处理大量连接的定时器
- **自动资源管理**：超时后自动delete Connection，无需手动管理

## 代码结构

```
day12/src/
├── main.cpp                    # 程序入口
├── net/                       # 网络领域
│   ├── Selector.h/cpp         # epoll封装
│   ├── Connection.h/cpp      # 客户端连接
│   └── Server.h/cpp          # TCP服务器
├── reactor/                   # Reactor领域
│   ├── EventHandler.h        # 事件处理器接口
│   ├── Reactor.h/cpp          # 事件分发中心（已集成ConnectionManager）
│   ├── ReactorPool.h/cpp      # Reactor线程池
│   └── ConnectionManager.h/cpp # 连接超时管理器
├── thread/                    # 线程池领域
│   ├── TaskQueue.h/cpp        # 任务队列
│   └── ThreadPool.h/cpp       # 线程池
└── timer/                     # 定时器领域
    ├── Timer.h                # 定时器接口
    ├── TimerHeap.h/cpp        # 最小堆定时器
    ├── TimerWheel.h/cpp       # 时间轮定时器
    └── TimerManager.h/cpp     # 定时器管理器
```

## ConnectionManager 接口

```cpp
class ConnectionManager {
public:
    ConnectionManager(Reactor* reactor, int64_t idleTimeoutMs = 30000);
    
    bool addConnection(Connection* conn);      // 注册连接
    void removeConnection(Connection* conn);  // 移除连接
    void updateActivity(Connection* conn);     // 更新活动时间
    size_t connectionCount() const;           // 连接数
    uint64_t timeoutCount() const;             // 超时断开数
    int64_t checkTimeouts();                   // 检查超时
};
```

## Reactor 变更

```cpp
class Reactor {
    // ...
    ConnectionManager m_connManager;  // 新增：连接管理器
    
    void registerHandler(EventHandler* handler) override;
    void removeHandler(EventHandler* handler) override;
    void loop() override;
};
```

## 编译运行

```bash
cd day12
make
sudo ./server 8080

# 测试超时
nc localhost 8080 &
# 等待35秒，观察日志
```

## 预期输出

```
========================================
  Day12: Connection Timeout Management
========================================

[Info] Connection timeout demo (30 seconds idle)
[Info] Connect with: nc localhost 8080
[Info] Then wait 30 seconds without sending data

[Server] Server listening on port 8080
[Reactor] Main Reactor started
[Reactor] Worker Reactor 0 started
...

# 连接后等待35秒无数据
[ConnectionManager] Connection timeout: 127.0.0.1:xxxxx (total timeouts: 1)
[Connection] Worker 0 closing: 127.0.0.1:xxxxx

# 发送数据后
[Connection] Worker 0 read 5 bytes from 127.0.0.1:xxxxx: hello
[Connection] Worker 0 remaining 0 bytes to 127.0.0.1:xxxxx
```

## 超时配置

- **默认超时**：30秒（30000ms）
- **配置方式**：修改Reactor构造函数中的`m_connManager(this, 30000)`
- **动态调整**：可在运行时修改idleTimeoutMs并重建定时器

## 下一步

Day13将学习日志系统：日志级别、格式化、输出目标。