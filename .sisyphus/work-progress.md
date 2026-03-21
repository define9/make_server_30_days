# Work Progress - 30天C++多路复用服务器

## 项目信息

- **开始日期**: 2026-03-20
- **进度**: 13/30 天
- **状态**: Day13完成

## 已完成内容

### Day01 - 阻塞I/O Echo服务器 ✓
- **目录**: `day01/src/`
- **文件**:
  - `Server.h/cpp`: TCP服务器类
  - `ClientHandler.h/cpp`: 客户端处理器类
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
- **功能**: 阻塞单客户端Echo，基础Socket编程
- **面向对象设计**: Server + ClientHandler分离，职责单一
- **编译状态**: ✓ 编译成功 (`g++ -std=c++11`)
- **可运行**: `cd day01 && make && ./server 8080`

### Day02 - select多路复用 ✓
- **目录**: `day02/src/`
- **文件**:
  - `Server.h/cpp`: 仅接受连接，不阻塞
  - `Selector.h/cpp`: select()系统调用封装
  - `Connection.h/cpp`: 客户端连接状态封装
  - `EventLoop.h/cpp`: 事件循环(核心调度器)
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
- **功能**: 单线程多路复用，同时处理多客户端
- **面向对象设计**:
  - Selector 封装fd_set管理和select等待
  - EventLoop 统一调度所有I/O事件
  - Connection 管理每个客户端连接状态
  - Server 只负责accept，解耦职责
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day02 && make && ./server 8080`

### Day03 - poll多路复用 ✓
- **目录**: `day03/src/`
- **文件**:
  - `Server.h/cpp`: 服务器(接受连接)
  - `Selector.h/cpp`: poll()系统调用封装（完全重写）
  - `EventLoop.h/cpp`: 事件循环
  - `Connection.h/cpp`: 客户端连接
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
- **功能**: 用poll()替代select()，解决FD_SETSIZE限制
- **核心改进**:
  - pollfd数组动态管理，无fd数量限制
  - 更清晰的API：pollfd结构比fd_set位图更直观
  - 跨平台支持（Linux、macOS）
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day03 && make && ./server 8080`

### Day04 - epoll基础(LT模式) ✓
- **目录**: `day04/src/`
- **文件**:
  - `Server.h/cpp`: 服务器
  - `Selector.h/cpp`: epoll()封装（完全重写）
  - `EventLoop.h/cpp`: 事件循环
  - `Connection.h/cpp`: 客户端连接
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
- **功能**: 用epoll()替代poll()，实现O(1)复杂度事件通知
- **核心改进**:
  - 内核使用红黑树管理fd，O(1)查找
  - 只返回已就绪的fd，无需遍历全部
  - Linux特有，高效
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day04 && make && ./server 8080`

### Day05 - epoll边缘触发(ET)模式 ✓
- **目录**: `day05/src/`
- **文件**:
  - `Server.h/cpp`: 服务器
  - `Selector.h/cpp`: epoll()封装 + EPOLLET标志
  - `EventLoop.h/cpp`: 事件循环（动态注册WRITE事件）
  - `Connection.h/cpp`: 客户端连接（非阻塞I/O + 循环读写）
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
- **功能**: ET模式 + 非阻塞I/O，高效事件处理
- **核心改进**:
  - ET模式：只在状态变化时通知一次
  - O_NONBLOCK：循环读写直到EAGAIN
  - **动态注册WRITE事件**：
    - 收到数据后，有数据要回复时注册WRITE
    - 发完数据后取消WRITE监听
    - 原因：ET模式下WRITE不会重复通知，必须按需注册
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day05 && make && ./server 8080`

### Day06 - 非阻塞I/O ✓
- **目录**: `day06/src/`
- **文件**:
  - `Server.h/cpp`: server socket O_NONBLOCK + 循环accept
  - `Selector.h/cpp`: epoll封装（无变化）
  - `EventLoop.h/cpp`: 事件循环（回调acceptAllConnections）
  - `Connection.h/cpp`: 非阻塞I/O（无变化）
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
  - `README.md`: Day06文档
- **功能**: 完整非阻塞服务器，server socket和accept都非阻塞
- **核心改进**:
  - **Server socket O_NONBLOCK**: server socket设置非阻塞
  - **acceptAllConnections()**: 循环accept直到EAGAIN
  - **多连接处理**: 多连接同时到达时一次性全部accept
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day06 && make && ./server 8080`

### Day07 - Reactor模式基础 ✓
- **目录**: `day07/src/`
- **文件**:
  - `EventHandler.h`: 事件处理器抽象接口
  - `Reactor.h/cpp`: 事件分发中心，管理Handler注册和事件分发
  - `Server.h/cpp`: 服务器 - 实现EventHandler
  - `Connection.h/cpp`: 连接 - 实现EventHandler
  - `Selector.h/cpp`: epoll封装
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
  - `README.md`: Day07文档
- **功能**: 引入Reactor模式，建立事件源和Handler注册机制
- **核心改进**:
  - **EventHandler接口**: 定义handleRead/handleWrite/handleClose/interest抽象方法
  - **Reactor类**: 事件分发中心，统一管理所有Handler的注册和事件分发
  - **解耦**: 事件源(Server)和事件处理(Connection)分离
  - **统一调度**: 所有Handler实现同一接口，事件分发逻辑一致
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day07 && make && ./server 8080`

### Day08 - 完整Reactor实现 ✓
- **目录**: `day08/src/`
- **文件**:
  - `EventHandler.h`: 事件处理器抽象接口（无变化）
  - `Reactor.h/cpp`: 事件分发中心 + 优雅关闭 + 连接统计
  - `Server.h/cpp`: 服务器 - 实现EventHandler
  - `Connection.h/cpp`: 连接 - 实现EventHandler
  - `Selector.h/cpp`: epoll封装（无变化）
  - `main.cpp`: 程序入口 - 显示统计信息
  - `Makefile`: 编译脚本
  - `README.md`: Day08文档
- **功能**: 完整Reactor实现，优雅关闭和连接统计
- **核心改进**:
  - **Reactor::closeAllHandlers()**: 事件循环退出前关闭所有Handler
  - **连接统计**: `totalConnections()` 总连接数，`activeConnections()` 活跃连接数
  - **增强日志**: 退出时显示统计信息
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day08 && make && ./server 8080`

### Day09 - 多线程Reactor ✓
- **目录**: `day09/src/`
- **文件**:
  - `EventHandler.h`: 事件处理器抽象接口（无变化）
  - `Reactor.h/cpp`: 事件分发中心（无变化）
  - `Server.h/cpp`: 服务器 - 实现EventHandler
  - `Connection.h/cpp`: 连接 - 实现EventHandler
  - `Selector.h/cpp`: epoll封装（无变化）
  - `ReactorPool.h/cpp`: Reactor线程池 - 管理主Reactor和Worker Reactor
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
  - `README.md`: Day09文档
- **功能**: 多线程Reactor，One Loop Per Thread
- **核心改进**:
  - **ReactorPool**: 管理主Reactor和Worker Reactor池
  - **主Reactor**: 仅处理accept，分发连接到Worker
  - **Worker Reactor**: 每个线程独立处理连接I/O
  - **负载均衡**: 轮询分配连接到Worker
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day09 && make && ./server 8080`

### Day10 - 线程池实现 ✓
- **目录**: `day10/src/`
- **文件**:
  - `EventHandler.h`: 事件处理器抽象接口（无变化）
  - `Reactor.h/cpp`: 事件分发中心（无变化）
  - `Server.h/cpp`: 服务器 - 实现EventHandler
  - `Connection.h/cpp`: 连接 - 实现EventHandler
  - `Selector.h/cpp`: epoll封装（无变化）
  - `ReactorPool.h/cpp`: Reactor线程池（无变化）
  - `TaskQueue.h/cpp`: 任务队列 - 线程安全的任务分发
  - `ThreadPool.h/cpp`: 线程池 - 生产者-消费者模式
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
  - `README.md`: Day10文档
- **功能**: 线程池实现，任务队列支持
- **核心改进**:
  - **TaskQueue**: 线程安全队列，push/pop操作
  - **ThreadPool**: 线程池，Worker线程从队列取任务执行
  - **生产者-消费者**: 主线程submit任务，Worker线程执行
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day10 && make && ./server 8080`

### Day11 - 定时器实现 ✓
- **目录**: `day11/src/`
- **文件**:
  - `EventHandler.h`: 事件处理器抽象接口（无变化）
  - `Reactor.h/cpp`: 事件分发中心（无变化）
  - `Server.h/cpp`: 服务器 - 实现EventHandler
  - `Connection.h/cpp`: 连接 - 实现EventHandler
  - `Selector.h/cpp`: epoll封装（无变化）
  - `ReactorPool.h/cpp`: Reactor线程池（无变化）
  - `TaskQueue.h/cpp`: 任务队列（无变化）
  - `ThreadPool.h/cpp`: 线程池（无变化）
  - `Timer.h`: 定时器接口
  - `TimerHeap.h/cpp`: 最小堆定时器实现
  - `TimerWheel.h/cpp`: 时间轮定时器实现
  - `TimerManager.h/cpp`: 定时器管理器
  - `main.cpp`: 程序入口
  - `Makefile`: 编译脚本
  - `README.md`: Day11文档
- **功能**: 定时器实现，支持超时检测
- **核心改进**:
  - **Timer接口**: 统一抽象addTimer/removeTimer/tick
  - **TimerHeap**: 最小堆实现，O(log n)效率
  - **TimerWheel**: 时间轮实现，O(1)添加/删除
  - **TimerManager**: 统一管理，支持切换不同实现
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day11 && make && ./server 8080`

### Day12 - 连接超时管理 ✓
- **目录**: `day12/src/`
- **文件**:
  - 继承Day11所有文件
  - `ConnectionManager.h/cpp`: 连接超时管理器 - 新增
  - `main.cpp`: 程序入口（更新）
  - `README.md`: Day12文档
- **功能**: 连接超时管理，空闲检测和资源回收
- **核心改进**:
  - **ConnectionManager**: 统一管理所有连接的定时器
  - **Reactor集成**: 事件循环支持超时检查
  - **连接生命周期**: 注册→活动→超时检测→断开→回收
  - **O(1)定时器重置**: 每次IO活动时更新定时器
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day12 && make && ./server 8080`

### Day13 - 基础日志系统 ✓
- **目录**: `day13/src/`
- **文件**:
  - 继承Day12所有文件
  - `log/Sink.h/cpp`: 输出目标抽象（ConsoleSink、FileSink）
  - `log/Formatter.h/cpp`: 日志格式化（%d %T %l %t %f %L %m）
  - `log/Logger.h/cpp`: 日志器核心（级别过滤、单例、宏定义）
  - `main.cpp`: 程序入口（集成日志）
  - `README.md`: Day13文档
- **功能**: 统一日志系统，支持多输出目标和格式化
- **核心改进**:
  - **Sink模式**: ConsoleSink/FileSink 分离输出目标
  - **日志级别**: TRACE/DEBUG/INFO/WARN/ERROR/FATAL
  - **格式化器**: 可配置格式串，支持时间/级别/文件/行号
  - **全局日志器**: 单例模式，LOG_INFO等宏方便调用
  - **全局替换**: 所有std::cout/std::cerr统一替换为LOG_*
- **编译状态**: ✓ 编译成功
- **可运行**: `cd day13 && make && ./server 8080`

## 设计要点符合用户要求

- ✓ **每天独立目录**: `day01/` ~ `day13/` 已创建
- ✓ **相互独立**: 每个目录都有完整的独立编译
- ✓ **无外部依赖**: 仅使用标准C++和Linux系统API
- ✓ **Makefile**: 每个目录都有独立Makefile
- ✓ **面向对象设计**: 每个类职责明确，接口清晰
- ✓ **详细注释**: 每个类、每个方法、关键代码行都有详细注释
- ✓ **渐进式**: 每天在前一天基础上增量改进

## 技术演进路线

```
Day01: 阻塞I/O（单客户端）
  ↓
Day02: select多路复用（解决多客户端，FD_SETSIZE=1024限制）
  ↓
Day03: poll多路复用（解决fd数量限制）
  ↓
Day04: epoll LT模式（O(1)复杂度，红黑树管理）
  ↓
Day05: epoll ET模式（最高效，只通知状态变化）
  ↓
Day06: 非阻塞I/O（server socket + accept完全非阻塞）
  ↓
Day07: Reactor模式基础（事件源 + 处理器注册）
  ↓
Day08: 完整Reactor实现（优雅关闭 + 连接统计）
  ↓
Day09: 多线程Reactor（One Loop Per Thread）
  ↓
Day10: 线程池实现（任务队列 + Worker）
  ↓
Day11: 定时器实现（最小堆 + 时间轮）
  ↓
Day12: 连接超时管理（空闲检测 + 资源回收）
  ↓
Day13: 基础日志系统（日志级别 + 格式化输出）
```

## 后续计划 (Day14 - Day30)

| Day | 主题 | 核心新增 |
|-----|------|----------|
| 14 | 配置系统 | 配置文件解析 |
| 15 | 信号处理 | 优雅退出 |
| 16 | 地址复用 | SO_REUSEADDR |
| 17 | TCP_NODELAY | 禁用Nagle |
| 18 | Proactor模式 | 异步I/O模型 |
| 19 | 事件循环原理 | libuv/libevent机制 |
| 20 | 自定义事件循环 | 跨平台抽象 |
| 21 | 连接池 | 连接复用 |
| 22 | HTTP服务器 | 协议解析 |
| 23 | HTTP长连接 | Keep-Alive |
| 24 | Chunked传输 | 分块响应 |
| 25 | SSL/TLS基础 | 加密通信 |
| 26 | OpenSSL集成 | 生产级安全 |
| 27 | 负载均衡 | 连接分配 |
| 28 | 反向代理 | 完整代理 |
| 29 | 性能优化 | benchmarks |
| 30 | 完整框架 | 架构整合 |

## 编译指令备忘

```bash
# 编译Day01
cd day01 && make clean && make && sudo ./server 8080

# 编译Day02
cd day02 && make clean && make && sudo ./server 8080

# 编译Day03
cd day03 && make clean && make && sudo ./server 8080

# 编译Day04
cd day04 && make clean && make && sudo ./server 8080

# 编译Day05
cd day05 && make clean && make && sudo ./server 8080

# 编译Day06
cd day06 && make clean && make && sudo ./server 8080

# 编译Day07
cd day07 && make clean && make && sudo ./server 8080

# 编译Day08
cd day08 && make clean && make && sudo ./server 8080

# 编译Day09
cd day09 && make clean && make && sudo ./server 8080

# 编译Day10
cd day10 && make clean && make && sudo ./server 8080

# 编译Day11
cd day11 && make clean && make && sudo ./server 8080

# 编译Day12
cd day12 && make clean && make && sudo ./server 8080

# 编译Day13
cd day13 && make clean && make && sudo ./server 8080

# 清理所有
for d in day{01,02,03,04,05,06,07,08,09,10,11,12,13}; do cd $d && make clean && cd ..; done
```

## 设计原则回顾

1. **无依赖**: 不使用boost、libevent等第三方库
2. **OOP**: 每个类只做一件事，封装合理
3. **注释**: 详细解释API和系统调用知识点
4. **渐进**: 每天在昨天基础上增量改进
5. **独立**: 每天都能独立编译运行
6. **复制起步**: 写新的一天时，先从上一天目录复制，确保内容连续性

---

*最后更新: 2026-03-21*
