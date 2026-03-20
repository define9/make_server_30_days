/**
 * @file ReactorPool.h
 * @brief Reactor线程池 - 管理主Reactor和Worker Reactor池
 * 
 * ==================== One Loop per Thread ====================
 * 
 * ReactorPool是Day09的核心组件，管理多线程Reactor架构：
 * 
 * 架构设计：
 * 1. 一个主Reactor：只在主线程运行，负责accept()新连接
 * 2. 多个Worker Reactor：分别在独立线程运行，负责处理已建立的连接
 * 
 * 连接分发流程：
 * [新连接] → [主Reactor accept] → [轮询选择Worker] → [Worker处理]
 * 
 * ==================== 负载均衡 ====================
 * 
 * 采用Round-Robin（轮询）策略：
 * - 新连接依次分配给Worker 0, 1, 2, 3, 0, 1, ...
 * - 简单高效，无状态
 * - 需要mutex保护m_nextReactor计数器
 */

#ifndef REACTOR_POOL_H
#define REACTOR_POOL_H

#include "Reactor.h"
#include "Server.h"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

/**
 * @brief Reactor线程池 - 多线程Reactor管理
 * 
 * 管理一个主Reactor和多个Worker Reactor：
 * - 主Reactor：运行在主线程，负责accept新连接
 * - Worker Reactor：运行在独立线程，负责处理连接
 * 
 * 使用流程：
 * 1. 构造函数创建所有Reactor
 * 2. start()启动Worker线程
 * 3. 主线程运行主Reactor.loop()
 * 4. stop()优雅停止所有Reactor
 */
class ReactorPool {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param numWorkers Worker Reactor数量
     * 
     * 创建主Reactor和所有Worker Reactor。
     * Worker暂时不运行，直到start()被调用。
     */
    ReactorPool(uint16_t port, int numWorkers);
    
    /**
     * @brief 析构函数
     */
    ~ReactorPool();

    // 禁用拷贝
    ReactorPool(const ReactorPool&) = delete;
    ReactorPool& operator=(const ReactorPool&) = delete;

    /**
     * @brief 启动Worker线程
     * 
     * 启动所有Worker Reactor，使其开始处理事件。
     * 主Reactor需要在主线程单独启动。
     */
    void start();

    /**
     * @brief 停止所有Reactor
     */
    void stop();

    /**
     * @brief 等待所有Worker线程退出
     */
    void waitForExit();

    /**
     * @brief 获取下一个Worker Reactor（轮询）
     * 
     * 线程安全的Round-Robin选择。
     * 用于主Reactor accept新连接后，分发给Worker。
     */
    Reactor* getNextWorker();

    /**
     * @brief 总连接数
     */
    uint64_t totalConnections() const;

private:
    std::unique_ptr<Reactor> m_mainReactor;      // 主Reactor（accept）
    std::vector<std::unique_ptr<Reactor>> m_workers;  // Worker Reactor池
    std::vector<std::thread> m_threads;           // Worker线程
    std::unique_ptr<Server> m_server;            // Server（主Reactor的事件源）
    
    int m_numWorkers;                            // Worker数量
    int m_nextReactor;                            // 轮询索引
    mutable std::mutex m_mutex;                   // 保护m_nextReactor
    std::atomic<bool> m_running;
};

#endif // REACTOR_POOL_H