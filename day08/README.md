# Day08: 完整Reactor实现

## 今日目标

在Day07基础上，完善Reactor实现，增加优雅关闭和连接统计功能。

## 核心改动

相比Day07的变化：
- **Reactor::closeAllHandlers()**：事件循环退出前关闭所有Handler
- **连接统计**：跟踪总连接数和活跃连接数
- **增强日志**：退出时显示统计信息

## 新学知识

### 1. 优雅关闭的重要性

服务器关闭时需要正确清理所有资源：

```
收到SIGINT/SIGTERM
       ↓
Reactor.stop()设置m_running=false
       ↓
下次wait()返回后退出循环
       ↓
Reactor.closeAllHandlers()关闭所有连接
       ↓
程序退出
```

如果不优雅关闭：
- 正在处理的客户端连接会被强制断开
- 可能丢失未发送的数据
- 文件描述符可能泄漏

### 2. 连接统计

Reactor添加了两个统计方法：
- `totalConnections()`：服务器运行期间处理的连接总数
- `activeConnections()`：当前活跃的连接数

### 3. 完整的事件循环

```
                    ┌─────────────────┐
                    │   Reactor.loop() │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│ selector.wait() │ │ handleEvents()  │ │closeAllHandlers()│
│   阻塞等待事件   │ │   分发到Handler  │ │  退出时清理所有  │
└────────┬────────┘ └────────┬────────┘ └────────┬────────┘
         │                   │                   │
         ▼                   ▼                   ▼
   [有事件发生]        [处理READ/WRITE]       [资源清理]
```

## 代码结构

```
day08/src/
├── main.cpp           # 程序入口 - 显示统计信息
├── EventHandler.h     # 事件处理器抽象接口
├── Reactor.h/cpp      # 事件分发中心 + 优雅关闭
├── Server.h/cpp       # 服务器 - 实现EventHandler
├── Connection.h/cpp   # 连接 - 实现EventHandler
└── Selector.h/cpp    # epoll封装
```

## 编译运行

```bash
cd day08
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
[Reactor] Started
[Server] Accepted client: 127.0.0.1:xxxxx (fd=5)
[Connection] Read total 5 bytes from 127.0.0.1:xxxxx: hello
[Connection] Closing: 127.0.0.1:xxxxx
[Reactor] Closing all handlers...
[Reactor] Stopped
========== Server Statistics ==========
Total connections handled: 1
======================================
Server stopped
```

## 下一步

Day09将学习多线程Reactor：One Loop per Thread模式。
