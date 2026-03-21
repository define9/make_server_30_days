# Day17: HTTP协议解析

## 今日目标

在Day16的Socket选项优化基础上，添加HTTP协议解析能力：
- HTTP请求解析（请求行、头部、Body）
- HTTP响应构建（状态码、头部、Body）
- 状态机解析器

## 核心改动

相比Day16的变化：
- **新增HttpRequest类**：解析HTTP请求方法、URL、版本、头部
- **新增HttpResponse类**：构建HTTP响应，支持常见状态码
- **新增HttpParser类**：状态机解析HTTP请求
- **Connection改造**：使用HttpParser解析请求，生成HTTP响应

## HTTP协议基础

### 请求格式

```
GET /path?query=value HTTP/1.1\r\n
Host: localhost:8080\r\n
Connection: keep-alive\r\n
\r\n
```

### 响应格式

```
HTTP/1.1 200 OK\r\n
Content-Type: text/plain\r\n
Content-Length: 13\r\n
\r\n
Hello World
```

## 解析状态机

```
┌─────────────────┐
│ READ_REQUEST_LINE │ ──CRLF──► READ_HEADERS
└─────────────────┘
         │
         ▼
┌─────────────────┐
│  READ_HEADERS   │ ──CRLF+CRLF──► READ_BODY
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   READ_BODY     │ ──完成──► PARSE_COMPLETE
└─────────────────┘
```

## 类结构

### HttpRequest

| 方法 | 说明 |
|------|------|
| parseRequestLine() | 解析请求行（方法、URL、版本）|
| addHeader() | 添加头部 |
| method() | 获取HTTP方法 |
| path() | 获取URL路径 |
| keepAlive() | 判断是否长连接 |
| contentLength() | 获取Body长度 |

### HttpResponse

| 方法 | 说明 |
|------|------|
| setStatus() | 设置状态码 |
| setBody() | 设置响应体 |
| setHeader() | 设置头部 |
| setKeepAlive() | 设置连接类型 |
| build() | 序列化为HTTP字符串 |

### HttpParser

| 状态 | 说明 |
|------|------|
| READ_REQUEST_LINE | 读取请求行 |
| READ_HEADERS | 读取头部 |
| READ_BODY | 读取Body |
| PARSE_COMPLETE | 解析完成 |
| PARSE_ERROR | 解析错误 |

## 代码结构

```
day17/src/
├── main.cpp                    # 程序入口
├── signal/                     # 信号处理领域
├── socket/                     # Socket选项领域
├── config/                     # 配置领域
├── net/                       # 网络领域
│   ├── Server.cpp           # 服务器
│   └── Connection.cpp       # 连接（使用HTTP解析）
├── http/                      # HTTP领域（新增）
│   ├── HttpRequest.h/cpp   # HTTP请求
│   ├── HttpResponse.h/cpp  # HTTP响应
│   └── HttpParser.h/cpp    # HTTP解析器
├── reactor/                   # Reactor领域
├── thread/                   # 线程池领域
├── timer/                    # 定时器领域
└── log/                      # 日志领域
```

## 测试

```bash
cd day17
make clean && make

# 启动服务器
sudo ./server

# 测试HTTP请求（另一个终端）
curl -v http://localhost:8080/

# 测试POST请求
curl -v -X POST http://localhost:8080/ -d "name=test"

# 测试带查询参数的请求
curl -v http://localhost:8080/api?foo=bar
```

## 预期输出

```
========================================
  Day17: HTTP Protocol Parsing
========================================
...
HTTP GET / from 127.0.0.1:xxxxx
HTTP POST / from 127.0.0.1:xxxxx
```

## 下一步

Day18将实现HTTP长连接：Keep-Alive、连接复用、Chunked分块传输。

## 参考

- RFC 7230 - HTTP/1.1 Message Syntax and Routing
- RFC 7231 - HTTP/1.1 Semantics and Content
