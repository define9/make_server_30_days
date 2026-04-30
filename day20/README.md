# Day20: 静态文件服务

## 今日目标

在Day19的分块传输基础上，实现静态文件服务：
- 文件读取：读取本地静态文件
- MIME类型：根据文件扩展名自动识别Content-Type
- URL映射：/static/路径映射到服务器本地文件

## 核心改动

相比Day19的变化：
- **新增MimeType工具类**：根据文件扩展名返回正确的MIME类型
- **新增FileUtil工具类**：文件读取和存在性检查
- **Connection改造**：支持/static/路径的静态文件服务

## 静态文件服务原理

### URL到文件路径的映射

```
URL: http://localhost:8080/static/index.html
     |-----------| |------|-----------|
     Server      Path     File
                           |
                           v
                   static/index.html
```

### MIME类型映射

```
.html -> text/html
.css  -> text/css
.js   -> application/javascript
.json -> application/json
.png  -> image/png
.jpg  -> image/jpeg
.gif  -> image/gif
.svg  -> image/svg+xml
.pdf  -> application/pdf
.txt  -> text/plain
... (更多类型参见MimeType.cpp)
```

### 文件读取流程

```
1. 解析HTTP请求，获取path
2. 检查path是否以/static/开头
3. 去掉前导'/'得到文件路径: static/index.html
4. 读取文件内容
5. 根据扩展名确定Content-Type
6. 构建HTTP响应并发送
```

## 代码结构

```
day20/src/
├── main.cpp                    # 程序入口
├── signal/                     # 信号处理领域
├── socket/                     # Socket选项领域
├── config/                     # 配置领域
├── net/                       # 网络领域
│   ├── Server.cpp           # 服务器
│   └── Connection.cpp       # 连接（支持静态文件服务）
├── http/                      # HTTP领域
│   ├── HttpRequest.h/cpp   # HTTP请求
│   ├── HttpResponse.h/cpp  # HTTP响应
│   └── HttpParser.h/cpp    # HTTP解析器
├── reactor/                   # Reactor领域
├── thread/                   # 线程池领域
├── timer/                    # 定时器领域
├── log/                      # 日志领域
└── util/                    # 工具领域
    ├── MimeType.h/cpp     # MIME类型映射
    └── FileUtil.h/cpp     # 文件操作工具
```

## 静态文件服务关键点

### 1. 文件路径安全性

```cpp
// URL: /static/../../../etc/passwd
// 危险！需要检查路径遍历攻击

// 简单方案：只允许/static/开头的路径
if (path.find("/static/") != 0) {
    // 不是静态文件请求
}
```

### 2. MIME类型缺省值

```cpp
// 未知文件类型使用octet-stream
static const std::string DEFAULT_TYPE = "application/octet-stream";
```

### 3. 文件读取失败处理

```cpp
if (!FileUtil::readFile(filePath, content)) {
    // 文件不存在或读取失败
    response.setStatus(HttpStatusCode::NOT_FOUND);
}
```

## 测试

```bash
cd day20
make clean && make

# 创建静态文件目录
mkdir -p static
echo "<html><body><h1>Hello Static!</h1></body></html>" > static/index.html
echo "body { color: blue; }" > static/style.css

# 启动服务器
sudo ./server

# 测试静态文件
curl -v http://localhost:8080/static/index.html
curl -v http://localhost:8080/static/style.css

```

## 预期输出

### 静态文件请求日志

```
[2026-04-30 17:20:26.132] [INFO] [src/net/Connection.cpp:143] HTTP GET /static/index.html from 127.0.0.1:44088
[2026-04-30 17:20:26.133] [DEBUG] [src/net/Connection.cpp:175] Worker 0 serving static file: static/index.html
```

### curl响应

```
< HTTP/1.1 200 OK
< Content-Type: text/html
< Connection: keep-alive

<html><body><h1>Hello Static!</h1></body></html>
```

## 下一步

Day21将实现请求路由：URL模式匹配、RESTful路由、路由参数提取。

## 参考

- MDN: Media Types - https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types
- RFC 7231 - HTTP/1.1 Semantics and Content