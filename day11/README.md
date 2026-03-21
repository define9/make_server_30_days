# Day11: 定时器实现 - Timer

## 今日目标

在Day10的线程池基础上，学习定时器实现：
- 最小堆定时器（TimerHeap）
- 时间轮定时器（TimerWheel）
- 定时器管理器（TimerManager）
- 集成到Reactor

## 核心改动

只搭了架子（定时器基础设施），Day12 才会用它做"连接超时管理"

相比Day10的变化：
- **新增Timer接口**：统一的定时器抽象
- **新增TimerHeap**：最小堆实现，适合大量定时器
- **新增TimerWheel**：时间轮实现，高效O(1)操作
- **新增TimerManager**：统一管理，支持切换不同实现
- **Reactor集成定时器**：事件循环支持定时任务

## 定时器算法对比

### 1. 最小堆（TimerHeap）

```
         100ms          ← 堆顶：最近到期
          /    \
      200ms   150ms
      /  \    /  \
    300 250 180 170
```

**优点：**
- 内存效率高：只需O(n)空间
- 精确度高：可以处理任意到期时间
- 实现简单：标准数据结构

**缺点：**
- 添加/删除是O(log n)
- 每次操作可能需要调整堆

**适用场景：**
- 定时器数量较少（<10000）
- 需要精确的任意时间到期
- 删除操作不频繁

### 2. 时间轮（TimerWheel）

```
         时间轮（环形）
           ↓
    ┌─────────────────┐
    │  Slot 0  → []  │
    │  Slot 1  → [t3] │
    │  Slot 2  → []  │  ← 指针每100ms转动一格
    │  Slot 3  → [t1,t2] │
    │   ...     ... │
    └─────────────────┘

t1,t2,t3 落到对应槽，到期时触发
```

**优点：**
- 添加/删除是O(1)：直接计算槽位置
-  tick推进极快：直接处理当前槽
- 适合大量定时器

**缺点：**
- 需要设置合理的tick和槽数
- 周期性定时器可能需要cascade到其他槽
- 内存开销比堆略大

**适用场景：**
- 定时器数量多（>10000）
- 定时器多为周期性（心跳检测）
- 到期时间分布均匀

### 3. 算法复杂度对比

| 操作 | 最小堆 | 时间轮 |
|------|--------|--------|
| 添加 | O(log n) | O(1) |
| 删除 | O(log n) | O(1) |
| 获取最近到期 | O(1) | O(1) |
| tick推进 | O(1) | O(1) |
| 内存 | O(n) | O(n + 槽数) |

### 4. 实际选择建议

```
定时器数量少 + 需要精确到期  → TimerHeap
定时器数量多 + 周期性为主   → TimerWheel
需要同时支持两种场景        → TimerManager (可切换)
```

## 代码结构

```
day11/src/
├── main.cpp                    # 程序入口
├── net/                       # 网络领域
│   ├── Selector.h/cpp         # epoll封装
│   ├── Connection.h/cpp       # 客户端连接
│   └── Server.h/cpp           # TCP服务器
├── reactor/                   # Reactor领域
│   ├── EventHandler.h         # 事件处理器接口
│   ├── Reactor.h/cpp          # 事件分发中心（已集成定时器）
│   └── ReactorPool.h/cpp      # Reactor线程池
├── thread/                    # 线程池领域
│   ├── TaskQueue.h/cpp        # 任务队列
│   └── ThreadPool.h/cpp       # 线程池
└── timer/                     # 定时器领域
    ├── Timer.h                # 定时器接口
    ├── TimerHeap.h/cpp        # 最小堆定时器
    ├── TimerWheel.h/cpp       # 时间轮定时器
    └── TimerManager.h/cpp     # 定时器管理器
```

## 定时器接口

```cpp
class Timer {
public:
    virtual bool addTimer(int64_t id, int64_t timeout, 
                         int64_t interval, TimerCallback callback) = 0;
    virtual bool removeTimer(int64_t id) = 0;
    virtual int64_t tick() = 0;
    virtual int64_t getNextExpire() const = 0;
    virtual size_t size() const = 0;
};
```

## 使用示例

```cpp
// 创建定时器（最小堆或时间轮）
TimerManager timer(TimerType::HEAP, 100);  // 100ms tick

// 添加一次性定时器（返回timer id，失败返回-1）
int64_t id = timer.addTimer(500, 0, [](int64_t timerId) {
    std::cout << "Timer " << timerId << " expired!" << std::endl;
});

// 添加周期性定时器
int64_t id2 = timer.addTimer(1000, 500, [](int64_t timerId) {
    std::cout << "Repeating timer " << timerId << " fired!" << std::endl;
});

// 删除定时器
timer.removeTimer(id);

// 主循环中调用tick()
while (running) {
    int64_t wait = timer.getNextExpire();
    // 等待事件...
    timer.tick();
}
```

## 编译运行

```bash
cd day11
make
sudo ./server 8080

# 测试
nc localhost 8080 &
echo "hello" | nc localhost 8080
```

## 预期输出

```
========================================
  Day11: Timer Implementation Demo
========================================

[Timer] Testing TimerHeap (Minimum Heap)...
[Timer] Added 3 timers, waiting...
[Timer] Timer 2 expired!
[Timer] Repeating timer 3 fired!
[Timer] One-shot timer 1 expired!
[Timer] Repeating timer 3 fired!
[Timer] Repeating timer 3 fired!

[Timer] Testing TimerWheel...
[Timer] Added 2 wheel timers, waiting...
[Timer] Wheel timer 4 expired!
[Timer] Wheel timer 5 expired!

========================================
  ReactorPool (Network I/O + Timer)
========================================
[Server] Server listening on port 8080
[Reactor] Main Reactor started
[Reactor] Worker Reactor 0 started
...
```

## 下一步

Day12将学习连接超时管理：空闲检测、资源回收。即对接定时器
