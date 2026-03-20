/**
 * @file EventLoop.h
 * @brief 事件循环 - 核心调度器
 *
 * EventLoop是整个服务器的核心：
 * 1. 持有Selector，进行I/O多路复用(poll)
 * 2. 管理所有Connection
 * 3. 提供给Server回调接口
 * 
 * Day03变化：Selector从select改为poll实现
 */

#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "Selector.h"
#include "Connection.h"
#include <map>
#include <functional>

/**
 * @brief 事件循环类
 *
 * 使用方法：
 * ```cpp
 * EventLoop loop;
 * loop.setNewConnectionCallback([](int fd) { ... });
 * loop.loop();
 * ```
 */
class EventLoop {
public:
    /**
     * @brief 构造函数
     */
    EventLoop();

    /**
     * @brief 析构函数
     */
    ~EventLoop();

    // 禁用拷贝
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    /**
     * @brief 启动事件循环
     * @param serverFd 服务器socket，添加到select监控
     */
    void loop(int serverFd);

    /**
     * @brief 停止事件循环
     */
    void stop();

    /**
     * @brief 添加连接
     */
    void addConnection(int clientSocket, const sockaddr_in& clientAddr);

    /**
     * @brief 移除连接
     */
    void removeConnection(int fd);

    /**
     * @brief 设置新连接回调
     */
    void setNewConnectionCallback(std::function<void(int)> cb) 
    { 
        m_newConnectionCallback = std::move(cb); 
    }

    /**
     * @brief 是否运行中
     */
    bool isRunning() const { return m_running; }

private:
    /**
     * @brief 处理事件
     */
    void handleEvents(int serverFd);

    Selector m_selector;                                      // poll封装
    std::map<int, Connection*> m_connections;                  // fd到连接的映射
    bool m_running;                                            // 运行状态
    std::function<void(int)> m_newConnectionCallback; // 新连接回调
    int m_serverFd;                                            // 服务器socket，由loop()传入
};

#endif // EVENT_LOOP_H
