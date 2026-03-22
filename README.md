# 25天实现C++ HTTP服务器

> 从零开始，无任何外部依赖，仅使用 make + C++

## 概述

本项目是一个25天的渐进式学习项目，每天一个独立目录。从基础的阻塞I/O服务器开始，演进到完整的多路复用服务器，最终实现一个支持长连接、Chunked传输、SSE的小型HTTP服务器。

## 在线对比

每天代码差异对比：[在线代码比较](https://define9.github.io/make_server_30_days/)

## 学习路线

| Day | 状态 | 主题 | 核心概念 |
|-----|------|------|----------|
| Day01 | ✅ | 阻塞I/O Echo服务器 | Socket编程基础、TCP连接、OOP封装 |
| Day02 | ✅ | select多路复用 | I/O多路复用、事件驱动、单线程多客户端 |
| Day03 | ✅ | poll多路复用 | poll接口、跨平台、fd数组管理 |
| Day04 | ✅ | epoll基础 | Linux特有、高效事件通知、LT模式 |
| Day05 | ✅ | epoll边缘触发(ET) | 高效模式、非阻塞I/O、惊群效应 |
| Day06 | ✅ | 非阻塞I/O | O_NONBLOCK、connect/accept/recv非阻塞 |
| Day07 | ✅ | Reactor模式基础 | 事件源、事件分发、Handler注册 |
| Day08 | ✅ | 完整Reactor实现 | 优雅关闭、连接统计 |
| Day09 | ✅ | 多线程Reactor | 线程安全、锁策略、负载均衡 |
| Day10 | ✅ | 线程池实现 | 任务队列、Worker线程、线程管理 |
| Day11 | ✅ | 定时器实现 | 最小堆、时间轮、过期连接处理 |
| Day12 | ✅ | 连接超时管理 | 空闲检测、资源回收 |
| Day13 | ✅ | 基础日志系统 | 日志级别、格式化、输出目标 |
| Day14 | ✅ | 配置系统 | 配置文件解析、参数可配置化 |
| Day15 | ✅ | 信号处理 | SIGTERM/SIGINT、优雅退出 |
| Day16 | ✅ | 网络选项优化 | SO_REUSEADDR、TCP_NODELAY |
| Day17 | ✅ | HTTP协议解析 | 请求/响应格式、状态码、Header解析 |
| Day18 | 📅 | HTTP长连接 | Keep-Alive、连接复用 |
| Day19 | 📅 | Chunked分块传输 | Transfer-Encoding: chunked、流式响应 |
| Day20 | 📅 | 静态文件服务 | 文件读取、Content-Type、MIME |
| Day21 | 📅 | 连接超时与优雅关闭 | 资源回收、收尾处理 |
| Day22 | 📅 | 整合测试 | 模块联调、边界情况 |
| Day23 | 📅 | 细节打磨 | 错误处理、边界条件 |
| Day24 | 📅 | SSE / Stream HTTP | text/event-stream、流式推送 |
| Day25 | 📅 | 完整HTTP服务器 | 架构整合、生产可用 |

## 当前进度

**68% 完成** (17/25 天)

已完成服务器核心I/O模型，进入HTTP协议栈阶段。

### 已掌握技能

```
✅ 基础网络编程        (Day01)
✅ I/O多路复用          (Day02-06)
✅ Reactor模式          (Day07-09)
✅ 线程池               (Day10)
✅ 定时器               (Day11-12)
✅ 日志系统             (Day13)
✅ 配置系统             (Day14)
✅ 信号处理             (Day15)
✅ 网络选项优化         (Day16)
✅ HTTP协议解析         (Day17)
```

### 下一步

Day18: HTTP长连接 (Keep-Alive、连接复用)

## 设计原则

1. **无外部依赖** - 所有代码仅使用标准C++库和系统API
2. **面向对象设计** - 每个类职责单一，抽象合理
3. **渐进式学习** - 每天的代码基于前一天，增加新功能
4. **详细注释** - 核心代码配有详细注释和解释
5. **独立可运行** - 每天的代码都可以单独编译运行

## 编译方式

```bash
# 每个目录都有独立的Makefile
cd day01
make
./server

# 或使用CMake (部分目录)
cd day02
mkdir build && cd build
cmake ..
make
./server
```

## 项目结构

```
make_server_30_days/
├── README.md              # 本文件
├── day01/                 # Day01: 阻塞I/O Echo服务器
│   ├── src/
│   │   ├── main.cpp
│   │   ├── Server.cpp
│   │   └── Server.h
│   ├── Makefile
│   └── README.md
├── day02/                 # Day02: select多路复用
│   └── ...
└── day25/                 # Day25: 完整HTTP服务器
    └── ...
```

## 每日Readme格式

每个目录都包含README.md，内容包括：
- **今日目标** - 这天要学什么
- **核心改动** - 相对于前一天做了什么
- **新学知识** - 涉及的新概念
- **代码结构** - 文件和类的说明
- **编译运行** - 如何编译和测试

## 致谢

本项目参考了：
- Unix网络编程 (UNP)
- Linux高性能服务器编程
- libuv/libevent源码
