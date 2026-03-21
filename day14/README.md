# Day14: 配置系统

## 今日目标

在Day13的日志系统基础上，添加配置系统：
- INI格式配置文件解析
- 配置项可配置化
- 命令行参数支持
- 默认值与配置覆盖

## 核心改动

相比Day13的变化：
- **新增ConfigFile类**：INI格式配置文件解析
- **新增Config类**：单例配置管理器，整合配置源
- **配置文件支持**：通过 `-c/--config` 指定配置文件
- **命令行参数**：支持 `-h/--help` 和 `--version`
- **验证机制**：配置加载后进行有效性验证

关于配置系统设计：
> 我们使用分层配置：默认配置 < 配置文件 < 命令行参数。Config类作为单例提供全局配置访问，ConfigFile负责INI解析。这样的设计便于扩展新的配置源（如环境变量）。

## 配置系统架构

```
                    ┌─────────────┐
                    │    main     │
                    └──────┬──────┘
                           │
                    ┌──────▼──────┐
                    │    Config   │  ← 单例，配置入口
                    └──────┬──────┘
                           │
              ┌────────────┴────────────┐
              ▼                         ▼
       ┌─────────────┐          ┌─────────────┐
       │ ConfigFile  │          │ CommandLine │
       │  (INI解析)   │          │  (参数解析)  │
       └─────────────┘          └─────────────┘
```

### 配置加载优先级

```
命令行参数 (-c config.conf)
    ↓ (覆盖)
配置文件 (server.conf)
    ↓ (覆盖)
默认值
```

## 配置文件格式

```ini
[server]
port = 8080
reactor_workers = 4
task_workers = 4
connection_timeout = 30
task_queue_size = 100

[log]
level = INFO
file = server.log
console = true
```

### 配置项说明

#### [server] 区块
| 配置项 | 类型 | 默认值 | 说明 |
|-------|------|--------|------|
| port | int | 8080 | 服务器监听端口 |
| reactor_workers | int | 4 | Reactor工作线程数 |
| task_workers | int | 4 | Task工作线程数 |
| connection_timeout | int | 30 | 连接超时时间（秒） |
| task_queue_size | int | 100 | 任务队列大小 |

#### [log] 区块
| 配置项 | 类型 | 默认值 | 说明 |
|-------|------|--------|------|
| level | string | INFO | 日志级别 |
| file | string | server.log | 日志文件名 |
| console | bool | true | 是否输出到控制台 |

## 代码结构

```
day14/src/
├── main.cpp                    # 程序入口
├── config/                     # 配置领域（新增）
│   ├── Config.h               # 配置管理器
│   ├── Config.cpp             # 配置管理器实现
│   ├── ConfigFile.h           # INI文件解析器
│   └── ConfigFile.cpp         # INI文件解析器实现
├── net/                       # 网络领域（继承自day13）
│   ├── Selector.h/cpp         # epoll封装
│   ├── Connection.h/cpp       # 客户端连接
│   └── Server.h/cpp           # TCP服务器
├── reactor/                   # Reactor领域（继承自day13）
│   ├── EventHandler.h         # 事件处理器接口
│   ├── Reactor.h/cpp          # 事件分发中心
│   ├── ReactorPool.h/cpp      # Reactor线程池
│   └── ConnectionManager.h/cpp # 连接超时管理器
├── thread/                    # 线程池领域（继承自day13）
│   ├── TaskQueue.h/cpp        # 任务队列
│   └── ThreadPool.h/cpp       # 线程池
├── timer/                     # 定时器领域（继承自day13）
│   ├── Timer.h                # 定时器接口
│   ├── TimerHeap.h/cpp        # 最小堆定时器
│   ├── TimerWheel.h/cpp       # 时间轮定时器
│   └── TimerManager.h/cpp     # 定时器管理器
└── log/                       # 日志领域（继承自day13）
    ├── Logger.h/cpp            # 日志器
    ├── Formatter.h/cpp         # 格式化器
    └── Sink.h/cpp              # Sink基类与实现
```

## ConfigFile 接口

```cpp
class ConfigFile {
public:
    bool load(const std::string& filepath);
    bool save(const std::string& filepath);

    std::string get(const std::string& section, const std::string& key,
                    const std::string& defaultValue = "") const;
    int getInt(const std::string& section, const std::string& key,
               int defaultValue = 0) const;
    bool getBool(const std::string& section, const std::string& key,
                  bool defaultValue = false) const;

    void set(const std::string& section, const std::string& key,
             const std::string& value);
};
```

## Config 接口

```cpp
class Config {
public:
    static Config& instance();

    bool loadFromFile(const std::string& filepath);
    bool loadFromArgs(int argc, char* argv[]);

    uint16_t port() const;
    int reactorWorkers() const;
    int taskWorkers() const;
    int connectionTimeout() const;
    int taskQueueSize() const;

    std::string logLevel() const;
    std::string logFile() const;
    bool consoleLog() const;

    void printUsage(const char* programName) const;
    bool validate() const;
};
```

## 编译运行

```bash
cd day14
make

# 使用默认配置运行
sudo ./server

# 使用配置文件
sudo ./server -c server.conf

# 查看帮助
./server --help
```

## 预期输出

```
========================================
  Day14: Configuration System
========================================
Config file: server.conf
Port: 8080
Reactor workers: 4
Task workers: 4
Connection timeout: 30 seconds
Task queue size: 100
Log level: INFO
Log file: server.log
Console log: enabled
```

## 下一步

Day15将学习信号处理：SIGTERM/SIGINT、优雅退出。
