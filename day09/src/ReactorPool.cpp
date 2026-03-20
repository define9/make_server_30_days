/**
 * @file ReactorPool.cpp
 * @brief ReactorPool实现
 */

#include "ReactorPool.h"
#include "Connection.h"
#include <iostream>
#include <chrono>

ReactorPool::ReactorPool(uint16_t port, int numWorkers)
    : m_numWorkers(numWorkers)
    , m_nextReactor(0)
    , m_running(false)
{
    m_mainReactor = std::make_unique<Reactor>();
    
    auto callback = [this]() -> Reactor* {
        return getNextWorker();
    };
    m_server = std::make_unique<Server>(port, m_mainReactor.get(), callback);
    
    for (int i = 0; i < m_numWorkers; ++i) {
        m_workers.push_back(std::make_unique<Reactor>());
        m_workers[i]->setWorkerId(i);
    }
}

/**
 * @brief 析构函数
 */
ReactorPool::~ReactorPool()
{
    stop();
}

/**
 * @brief 启动所有Worker线程
 */
void ReactorPool::start()
{
    m_running = true;
    
    // 创建并启动Worker线程
    for (int i = 0; i < m_numWorkers; ++i) {
        m_threads.emplace_back([this, i]() {
            // 每个Worker在独立线程中运行自己的Reactor事件循环
            std::cout << "[Reactor] Worker Reactor " << i << " started" << std::endl;
            m_workers[i]->loop();
            std::cout << "[Reactor] Worker Reactor " << i << " stopped" << std::endl;
        });
    }
    
    std::cout << "[Reactor] Main Reactor started" << std::endl;
}

/**
 * @brief 停止所有Reactor
 */
void ReactorPool::stop()
{
    if (!m_running) return;
    m_running = false;
    
    // 停止主Reactor（主线程会调用）
    m_mainReactor->stop();

    // 停止子Reactor
    for (int i = 0; i < m_numWorkers; i++) {
        m_workers[i]->stop();
    }
}

/**
 * @brief 等待所有Worker线程退出
 * 
 * 主线程运行主Reactor事件循环，
 * 所有Worker线程在独立线程中运行。
 */
void ReactorPool::waitForExit()
{
    // 主Reactor运行在主线程
    // 注意：stop()已经被调用，这里loop()会在stop()后返回
    m_mainReactor->loop();
    
    // 等待所有Worker线程结束
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "[Reactor] Main Reactor stopped" << std::endl;
}

/**
 * @brief 获取下一个Worker Reactor（轮询）
 * 
 * 线程安全的Round-Robin选择。
 * 主Reactor accept新连接后调用此方法获取目标Worker。
 */
Reactor* ReactorPool::getNextWorker()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Reactor* worker = m_workers[m_nextReactor].get();
    m_nextReactor = (m_nextReactor + 1) % m_numWorkers;
    return worker;
}

/**
 * @brief 总连接数
 */
uint64_t ReactorPool::totalConnections() const
{
    uint64_t total = 0;
    for (const auto& worker : m_workers) {
        total += worker->totalConnections();
    }
    return total;
}