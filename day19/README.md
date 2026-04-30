# Day19: Chunked分块传输

## 今日目标

在Day18的长连接基础上，实现HTTP分块传输编码（Transfer-Encoding: chunked）：
- 流式响应：无需预先知道Content-Length即可发送数据
- Chunked格式：正确的十六进制大小块 + 数据块格式
- 长连接 + 分块：两者的完美结合

## 核心改动

相比Day18的变化：
- **HttpResponse改造**：添加chunked编码支持
- **分块响应发送**：handleWrite中实现分块流式发送
- **分块请求解析**：HttpParser支持解析chunked请求

## Chunked编码原理

### 什么是Chunked编码

当服务器无法预知响应体大小时（如动态生成内容、流式数据），可以使用chunked编码：

```
HTTP/1.1 200 OK
Transfer-Encoding: chunked

5\r\n          # 第一个chunk：5字节
hello\r\n
6\r\n          # 第二个chunk：6字节
 world\r\n
0\r\n         # 结束chunk：0字节
\r\n          # 最后一个空行
```

### Chunked响应格式

```
HTTP/1.1 200 OK
Transfer-Encoding: chunked
（或使用 Content-Length 但不能与 chunked 共用）

[size in hex]\r\n
[data]\r\n
[size in hex]\r\n
[data]\r\n
...
0\r\n
\r\n
```

### 解析过程

```
分块数据: "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n"

解析步骤:
1. 读取chunk大小: "5" = 5字节
2. 读取5字节数据: "hello"
3. 读取下一个chunk大小: "6" = 6字节
4. 读取6字节数据: " world"
5. 读取结束chunk: "0" = 0字节
6. 读取结束空行: "\r\n"
```

## 代码结构

```
day19/src/
├── main.cpp                    # 程序入口
├── signal/                     # 信号处理领域
├── socket/                     # Socket选项领域
├── config/                     # 配置领域
├── net/                       # 网络领域
│   ├── Server.cpp           # 服务器
│   └── Connection.cpp       # 连接（支持chunked流式发送）
├── http/                      # HTTP领域
│   ├── HttpRequest.h/cpp   # HTTP请求（支持chunked解析）
│   ├── HttpResponse.h/cpp  # HTTP响应（支持chunked编码）
│   └── HttpParser.h/cpp    # HTTP解析器（支持chunked解析）
├── reactor/                   # Reactor领域
├── thread/                   # 线程池领域
├── timer/                    # 定时器领域
└── log/                      # 日志领域
```

## Chunked响应关键点

### 1. 不能同时使用Content-Length和Chunked

```cpp
// 错误：同时设置
response.setContentLength(100);
response.setHeader("Transfer-Encoding", "chunked");  // 冲突！

// 正确：使用chunked时不设置Content-Length
response.setHeader("Transfer-Encoding", "chunked");
// 不设置Content-Length
```

### 2. 分块发送需要状态跟踪

```cpp
// Connection中需要新增状态
bool m_chunkedMode;           // 是否使用分块模式
std::string m_currentChunk;    // 当前待发送的分块数据
size_t m_chunkSentBytes;       // 当前分块已发送字节数
```

### 3. 分块结束需要发送终止块

```
正常结束: "0\r\n\r\n"
不能简单关闭连接（需要graceful finish）
```

## Chunked解析关键点

### 1. 大小行格式

```
"[0-9a-fA-F]+\r\n"

例如:
- "5\r\n" = 5字节
- "1a\r\n" = 26字节 (十六进制)
- "1A\r\n" = 26字节 (大小写不敏感)
- "0\r\n" = 结束chunk
```

### 2. 结束chunk后还有trailer（可选）

```
0\r\n
\r\n                    # 结束chunk
[optional trailer]     # 额外的header，极少使用
```

### 3. 解析状态机扩展

```
READ_BODY:
  - Content-Length > 0: 读取指定字节数
  - Transfer-Encoding: chunked: 分块解析模式

CHUNK_SIZE: 读取十六进制大小行
CHUNK_DATA: 读取指定字节数的数据
CHUNK_END: 读取 "\r\n"
CHUNK_TRAILER: 可选的trailer header
CHUNK_DONE: 解析完成
```

## 测试

```bash
cd day19
make clean && make

# 启动服务器
sudo ./server

# 测试chunked响应
curl -v -k http://localhost:8080/chunked

# 测试普通长连接
curl -v -k http://localhost:8080/

# 测试chunked请求（发送分块数据）
curl -v -k -X POST http://localhost:8080/ \
  -H "Transfer-Encoding: chunked" \
  -d "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n"
```

## 预期输出

### Chunked请求解析日志

```
[2026-04-30 17:20:26.132] [INFO] [src/net/Server.cpp:96] Accepted client: 127.0.0.1:44088 (fd=10)
[2026-04-30 17:20:26.133] [DEBUG] [src/net/Server.cpp:110] -> Worker 0
[2026-04-30 17:20:26.133] [DEBUG] [src/net/Connection.cpp:118] Worker 0 read 204 bytes from 127.0.0.1:44088
[2026-04-30 17:20:26.133] [INFO] [src/net/Connection.cpp:143] HTTP POST / from 127.0.0.1:44088
```

**注意**：Chunked请求 `5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n` 在一次 `read()` 中完成解析（204 bytes），包括末尾的 `\r\n`。

### curl响应

```
< HTTP/1.1 200 OK
< Connection: keep-alive
< Content-Length: 192
< Content-Type: text/plain

HTTP Request Received
Method: POST
URL: /
Path: /
HTTP Version: 1.1
Connection: keep-alive

Headers:
  accept: */*
  host: localhost:8080
  transfer-encoding: chunked
  user-agent: curl/8.5.0
```

## 下一步

Day20将实现静态文件服务：文件读取、Content-Type、MIME类型。

## 参考

- RFC 7230 - HTTP/1.1 Message Syntax and Routing (Section 4.1)
- RFC 7231 - HTTP/1.1 Semantics and Content