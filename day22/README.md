# Day22: 整合测试

## 今日目标

模块联调与边界情况测试：
- 验证所有模块（Router、HTTP解析、静态文件、长连接）协同工作
- 边界情况处理：空请求、超大Header、错误路径
- 压力测试：短时间大量并发请求

## 核心改动

相比Day21的变化：
- **边界测试路由**：新增多种边界情况测试端点
- **错误处理增强**：完善各种异常情况处理
- **日志优化**：更清晰的测试输出

## 测试架构

```
客户端请求
    │
    ├──► 静态文件  (/static/*)
    │       └── FileUtil + MimeType
    │
    ├──► API路由   (/api/*)
    │       └── SimpleRouter::route()
    │
    └──► 边界测试  (/test/*)
            └── 各种边界情况
```

## 边界测试端点

| 端点 | 测试内容 |
|------|----------|
| `/test/empty` | 空路径处理 |
| `/test/big-header` | 超大Header |
| `/test/chunked` | 分块传输测试 |
| `/test/echo` | 请求echo（用于调试） |

## 测试命令

```bash
cd day22
make clean && make
sudo ./server

# 边界测试
curl http://localhost:8080/test/empty
curl http://localhost:8080/test/big-header
curl -X POST http://localhost:8080/test/chunked -d "hello world"
curl http://localhost:8080/test/echo?msg=hello

# 长连接测试
curl -v http://localhost:8080/api/status
curl -v http://localhost:8080/api/users/123

# 静态文件
curl http://localhost:8080/static/index.html

# 并发测试
for i in {1..50}; do
    curl http://localhost:8080/api/status &
done
wait
```

## 预期输出

```
[INFO] HTTP GET /api/users/123 from 127.0.0.1:xxx
[INFO] HTTP POST /test/chunked from 127.0.0.1:xxx
[INFO] Registered 6 routes
```

## 集成测试检查清单

- [x] Router正常匹配
- [x] 静态文件正常读取
- [x] 404返回正确
- [x] 长连接复用
- [x] 并发处理