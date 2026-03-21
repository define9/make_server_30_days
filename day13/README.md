# Day13: 基础日志系统

## 今日目标

在Day12的连接超时管理基础上，添加统一的日志系统：
- 日志级别控制
- 格式化输出
- 多输出目标（控制台、文件）

## 核心改动

相比Day12的变化：
- **新增Logger类**：统一管理日志输出
- **日志Sink抽象**：支持控制台和文件输出
- **日志级别**：TRACE/DEBUG/INFO/WARN/ERROR/FATAL
- **格式化器**：可配置的日志格式
- **全局日志器**：方便各模块调用

关于日志系统设计：
> 我们使用Sink模式分离日志目标和格式化逻辑。Logger负责日志级别过滤和分发，Sink负责具体输出，Formatter负责格式编排。这样的设计便于扩展新的输出目标（如syslog、网络等）。

## 日志系统架构

```
                    ┌─────────────┐
                    │   Logger    │  ← 统一入口，日志级别过滤
                    └──────┬──────┘
                           │
                    ┌──────▼──────┐
                    │   Formatter │  ← 格式编排
                    └──────┬──────┘
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
       ┌────────────┐           ┌────────────┐
       │ConsoleSink  │           │  FileSink   │
       └────────────┘           └────────────┘
```

### 日志级别

| 级别 | 值 | 用途 |
|------|-----|------|
| TRACE | 0 | 详细跟踪信息 |
| DEBUG | 1 | 调试信息 |
| INFO | 2 | 一般信息 |
| WARN | 3 | 警告信息 |
| ERROR | 4 | 错误信息 |
| FATAL | 5 | 致命错误 |

### 格式串

- `%d` - 日期时间
- `%T` - 时间戳（毫秒）
- `%l` - 日志级别
- `%t` - 线程ID
- `%f` - 源文件名
- `%L` - 源行号
- `%m` - 日志消息

## 代码结构

```
day13/src/
├── main.cpp                    # 程序入口
├── net/                       # 网络领域（继承自day12）
│   ├── Selector.h/cpp         # epoll封装
│   ├── Connection.h/cpp      # 客户端连接
│   └── Server.h/cpp          # TCP服务器
├── reactor/                   # Reactor领域（继承自day12）
│   ├── EventHandler.h        # 事件处理器接口
│   ├── Reactor.h/cpp          # 事件分发中心
│   ├── ReactorPool.h/cpp      # Reactor线程池
│   └── ConnectionManager.h/cpp # 连接超时管理器
├── thread/                    # 线程池领域（继承自day12）
│   ├── TaskQueue.h/cpp        # 任务队列
│   └── ThreadPool.h/cpp       # 线程池
├── timer/                     # 定时器领域（继承自day12）
│   ├── Timer.h                # 定时器接口
│   ├── TimerHeap.h/cpp        # 最小堆定时器
│   ├── TimerWheel.h/cpp       # 时间轮定时器
│   └── TimerManager.h/cpp     # 定时器管理器
└── log/                       # 日志领域（新增）
    ├── Logger.h               # 日志器
    ├── Logger.cpp             # 日志器实现
    ├── Formatter.h            # 格式化器
    ├── Formatter.cpp          # 格式化器实现
    └── Sink.h                 # Sink基类与实现
```

## Logger 接口

```cpp
class Logger {
public:
    enum Level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };
    
    Logger(Level level = INFO);
    
    void setLevel(Level level);
    void addSink(std::shared_ptr<Sink> sink);
    
    void log(Level level, const char* file, int line, const char* fmt, ...);
    
    void trace(const char* file, int line, const char* fmt, ...);
    void debug(const char* file, int line, const char* fmt, ...);
    void info(const char* file, int line, const char* fmt, ...);
    void warn(const char* file, int line, const char* fmt, ...);
    void error(const char* file, int line, const char* fmt, ...);
    void fatal(const char* file, int line, const char* fmt, ...);
};
```

## Sink 接口

```cpp
class Sink {
public:
    virtual ~Sink() = default;
    virtual void write(const std::string& formatted) = 0;
};

class ConsoleSink : public Sink { ... };
class FileSink : public Sink { ... };
```

## 编译运行

```bash
cd day13
make
sudo ./server 8080

# 测试日志
nc localhost 8080 &
# 发送数据，观察日志输出
```

## 预期输出

```
========================================
  Day13: Logging System
========================================

[2024-01-01 12:00:00.123] [INFO] [main.cc:25] Server listening on port 8080
[2024-01-01 12:00:00.124] [INFO] [main.cc:32] ThreadPool started with 4 workers
[2024-01-01 12:00:01.456] [DEBUG] [ReactorPool.cpp:42] Worker 0 started
[2024-01-01 12:00:05.789] [INFO] [Server.cpp:87] Accepted client: 127.0.0.1:54321
[2024-01-01 12:00:05.790] [DEBUG] [Connection.cpp:105] Worker 0 read 5 bytes: hello
```

## 配置说明

```cpp
// 创建日志器，默认级别为INFO
Logger logger(Logger::INFO);

// 添加控制台输出（默认）
logger.addSink(std::make_shared<ConsoleSink>());

// 添加文件输出
logger.addSink(std::make_shared<FileSink>("server.log"));

// 使用宏方便调用
LOG_INFO("Server started on port %d", 8080);
LOG_ERROR("Connection failed: %s", strerror(errno));
```

## 下一步

Day14将学习配置系统：配置文件解析、参数可配置化。
