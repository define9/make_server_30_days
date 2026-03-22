# Day18: HTTP长连接

## 今日目标

在Day17的HTTP协议解析基础上，实现HTTP长连接（Keep-Alive）支持：
- 连接复用：同一个TCP连接处理多个HTTP请求
- Keep-Alive机制：减少连接建立/关闭的开销
- 正确的状态管理：解析完一个请求后不立即清空缓冲区

## 核心改动

相比Day17的变化：
- **Connection改造**：不再立即清空缓冲区和重置解析器
- **长连接支持**：处理完请求后检查Connection头，决定是否保持连接
- **剩余数据处理**：一个请求处理完后，缓冲区可能还有下一个请求的数据

## HTTP长连接原理

### 短连接 vs 长连接

```
短连接 (HTTP/1.0, Connection: close):
  Client                    Server
    |--- TCP Handshake ----->|
    |--- HTTP Request ------>|
    |<-- HTTP Response ------|
    |<-- TCP FIN ------------|
    |--- TCP FIN ----------->|
    (连接关闭，下次请求需重新建立TCP)

长连接 (HTTP/1.1, Connection: keep-alive):
  Client                    Server
    |--- TCP Handshake ----->|
    |--- HTTP Request 1 ---->|
    |<-- HTTP Response 1 ----|
    |--- HTTP Request 2 ---->|
    |<-- HTTP Response 2 ----|
    |--- HTTP Request 3 ---->|
    |<-- HTTP Response 3 ----|
    ... (继续复用)
    |<-- TCP FIN ------------|
    |--- TCP FIN ----------->|
```

### Keep-Alive行为

HTTP/1.1默认开启Keep-Alive，除非显式声明`Connection: close`。
HTTP/1.0需要双方都声明`Connection: keep-alive`才开启长连接。

## 长连接处理流程

```
handleRead():
  1. 读取数据到 m_readBuffer
  2. 调用 m_httpParser.parse() 解析
  3. if hasCompleteRequest():
       处理请求，生成响应
       if keep-alive:
         不清空buffer，不重置parser（可能还有下一个请求的数据）
         继续等待更多数据
       else:
         关闭连接
```

## 代码结构

```
day18/src/
├── main.cpp                    # 程序入口
├── signal/                     # 信号处理领域
├── socket/                     # Socket选项领域
├── config/                     # 配置领域
├── net/                       # 网络领域
│   ├── Server.cpp           # 服务器
│   └── Connection.cpp       # 连接（支持长连接复用）
├── http/                      # HTTP领域
│   ├── HttpRequest.h/cpp   # HTTP请求
│   ├── HttpResponse.h/cpp  # HTTP响应
│   └── HttpParser.h/cpp    # HTTP解析器
├── reactor/                   # Reactor领域
├── thread/                   # 线程池领域
├── timer/                    # 定时器领域
└── log/                      # 日志领域
```

## 长连接关键点

### 1. 不立即清空缓冲区

```cpp
// 旧代码 (错误)
if (m_httpParser.hasCompleteRequest()) {
    processHttpRequest();
    m_readBuffer.clear();      // 错误：可能还有下一个请求
    m_httpParser.reset();      // 错误：需要等响应发送完
}
```

### 2. 正确处理剩余数据

```
假设客户端发送:
  "GET /first HTTP/1.1\r\n\r\nGET /second HTTP/1.1\r\n\r\n"

buffer可能是:
  "GET /first HTTP/1.1\r\n\r\nGET /second HTTP/1.1\r\n\r\n"
                       ↑
              处理完第一个请求后，指针停在这里
              剩余数据 "GET /second..." 等待下次parse
```

### 3. 响应发送完再重置

只有当响应完全发送出去后，才能重置parser准备下一个请求。

## 测试

```bash
cd day18
make clean && make

# 启动服务器
sudo ./server

# 测试长连接 (多个请求复用同一个连接)
curl -v -k http://localhost:8080/
curl -v -k http://localhost:8080/api?foo=bar
curl -v -k -X POST http://localhost:8080/ -d "name=test"

# 使用-p保留连接测试
curl -v -k -p http://localhost:8080/

# 测试短连接 (Connection: close)
curl -v -k -H "Connection: close" http://localhost:8080/
```

## 预期输出

```
========================================
  Day18: HTTP Long Connection
========================================
...
HTTP GET / from 127.0.0.1:xxxxx
HTTP GET /api?foo=bar from 127.0.0.1:xxxxx  (同一连接)
HTTP POST / from 127.0.0.1:xxxxx          (同一连接)
```

## 下一步

Day19将实现Chunked分块传输：Transfer-Encoding: chunked、流式响应。

## 参考

- RFC 7230 - HTTP/1.1 Message Syntax and Routing
- RFC 7231 - HTTP/1.1 Semantics and Content
- RFC 7232 - HTTP/1.1 Conditional Requests
