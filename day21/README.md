# Day21: 请求路由

## 今日目标

实现简单高效的请求路由：
- URL模式匹配：支持 `/user/:id` 格式的动态路由
- 共享Router：全局单例，所有Connection共用
- 路由注册：在main.cpp中集中注册

## 核心改动

相比Day20的变化：
- **新增SimpleRouter**：极简路由实现，约80行代码
- **共享单例**：SimpleRouter::instance() 全局唯一
- **Connection改造**：调用共享Router，无需持有副本

## 架构设计

```
main.cpp                    Server                     Connection
   │                          │                           │
   ├── 创建Router              │                           │
   ├── 注册路由 ──────────────► │                           │
   │                          │                           │
   │                          └── 新连接进来时 ───────────► │
   │                          │         共用同一Router    │
   ▼                          ▼                           ▼
SimpleRouter单例              用同一份Router               用同一份Router
```

## SimpleRouter特性

- **单例模式**：全局共享一份路由表，内存高效
- **简单匹配**：不用regex，按`:`分割字符串比较
- **链式API**：get/post/put/del 方便注册
- **约80行代码**：简洁易懂

## 代码结构

```
day21/src/
├── main.cpp                   # 路由注册在这里！
├── http/
│   ├── SimpleRouter.h/cpp    # 极简路由 ~80行
│   └── ...
└── net/
    └── Connection.cpp        # 调用 SimpleRouter::instance()
```

## 路由注册示例

```cpp
// main.cpp
int main() {
    auto& router = SimpleRouter::instance();

    router.get("/api/users/:id", [](const HttpRequest& req, HttpResponse& resp) {
        resp.setStatus(HttpStatusCode::OK);
        resp.setContentType("application/json");
        resp.setBody("{\"path\": \"" + req.path() + "\"}");
    });

    router.get("/api/status", [](const HttpRequest& req, HttpResponse& resp) {
        resp.setStatus(HttpStatusCode::OK);
        resp.setBody("OK");
    });

    router.post("/api/data", [](const HttpRequest& req, HttpResponse& resp) {
        resp.setStatus(HttpStatusCode::CREATED);
        resp.setBody("Created: " + req.body());
    });

    ReactorPool reactorPool(...);
    reactorPool.start();
}
```

## 路由匹配规则

| Pattern | URL | 匹配 |
|---------|-----|------|
| `/api/users/:id` | `/api/users/123` | ✅ |
| `/api/users/:id` | `/api/users/` | ❌ |
| `/api/status` | `/api/status` | ✅ |
| `/api/status` | `/api/users` | ❌ |

## 测试

```bash
cd day21
make clean && make

# 启动服务器
sudo ./server

# 测试路由
curl http://localhost:8080/api/users/123
curl http://localhost:8080/api/status
curl -X POST http://localhost:8080/api/data -d "hello"

# 静态文件仍支持
curl http://localhost:8080/static/index.html
```

## 预期输出

```
[INFO] Registered 3 routes
[INFO] HTTP GET /api/users/123 from 127.0.0.1:xxx
```