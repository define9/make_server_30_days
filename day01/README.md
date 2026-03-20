# Day01: 阻塞I/O Echo服务器

## 今日目标

学习Socket编程基础，实现一个最简单的TCP Echo服务器。

## 核心改动

- **从零开始**：建立项目结构
- **面向对象设计**：封装Server类、ClientHandler类

## 新学知识

### 1. Socket编程基础

**什么是Socket？**
Socket是网络通信的端点，用于描述IP地址和端口号。本质上是一种文件描述符。

**TCP Socket通信流程：**

```
Server端:
socket() -> bind() -> listen() -> accept() -> read()/write() -> close()
                                                        ^
                                                        |
Client端:                                               |
socket() -------------------------------------------> connect() -> read()/write() -> close()
```

### 2. 阻塞I/O vs 非阻塞I/O

- **阻塞I/O**: 调用read()/write()时，如果没有数据或缓冲区满，进程会休眠等待
- **非阻塞I/O**: 立即返回，要么读到数据，要么返回EAGAIN

### 3. 面向对象封装

本项目将网络通信封装为两个核心类：

- **Server**: 负责监听端口、接受新连接
- **ClientHandler**: 负责与单个客户端通信

## 代码结构

```
day01/src/
├── main.cpp        # 程序入口
├── Server.h        # Server类声明
├── Server.cpp      # Server类实现
├── ClientHandler.h # ClientHandler类声明
└── ClientHandler.cpp # ClientHandler类实现
```

### 类设计

```
Server
├── m_port: int                 // 监听端口
├── m_serverSocket: int         // 服务器socket文件描述符
├── start(): void              // 启动服务器
├── acceptClient(): void       // 接受客户端连接
└── ~Server(): void            // 清理资源

ClientHandler
├── m_clientSocket: int         // 客户端socket文件描述符
├── m_clientAddr: sockaddr_in   // 客户端地址
├── handle(): void              // 处理客户端请求(echo)
└── ~ClientHandler(): void     // 关闭连接
```

## 编译运行

```bash
# 编译
cd day01
make

# 运行(需要root权限或选择1024以上端口)
sudo ./server 8080

# 测试(另开终端)
telnet localhost 8080
# 或者
nc localhost 8080
# 输入任意文字，会echo回显
```

## 预期输出

```
[Server] Server listening on port 8080
[Server] Waiting for client connection...
[Server] Accepted client from 127.0.0.1:xxxxx
[Handler] Echo: Hello
[Handler] Echo: World
[Handler] Client disconnected
```

## 注意事项

1. 本版本是**阻塞I/O**，一次只能处理一个客户端
2. 服务器socket使用`SOCK_STREAM`（面向连接、可靠）
3. 使用`htons()`转换端口号为网络字节序
4. `atoi()`将字符串端口号转为整数

## 下一步

明天我们将学习`select`多路复用，实现同时处理多个客户端！
