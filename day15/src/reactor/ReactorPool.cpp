/**
 * @file ReactorPool.cpp
 * @brief ReactorPool实现
 */

#include "ReactorPool.h"
#include "net/Server.h"
#include "log/Logger.h"
#include <chrono>

ReactorPool::ReactorPool(uint16_t port, int numWorkers)
    : m_numWorkers(numWorkers)
    , m_nextReactor(0)
    , m_running(false)
{
    m_mainReactor = std::make_unique<Reactor>();
    
    // 获取轮询
    auto callback = [this]() -> Reactor* {
        return getNextWorker();
    };
    // Server会把自己注册到 m_mainReactor
    m_server = std::make_unique<Server>(port, m_mainReactor.get(), callback);
    
    for (int i = 0; i < m_numWorkers; ++i) {
        m_workers.push_back(std::make_unique<Reactor>());
        m_workers[i]->setWorkerId(i);
    }
}

ReactorPool::~ReactorPool()
{
    stop();
}

void ReactorPool::start()
{
    m_running = true;
    
    for (int i = 0; i < m_numWorkers; ++i) {
        m_threads.emplace_back([this, i]() {
            LOG_INFO("Worker Reactor %d started", i);
            m_workers[i]->loop();
            LOG_INFO("Worker Reactor %d stopped", i);
        });
    }
    
    LOG_INFO("Main Reactor started");
}

void ReactorPool::stop()
{
    if (!m_running) return;
    m_running = false;
    
    m_mainReactor->stop();

    for (int i = 0; i < m_numWorkers; i++) {
        m_workers[i]->stop();
    }
}

void ReactorPool::waitForExit()
{
    m_mainReactor->loop();
    
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    LOG_INFO("Main Reactor stopped");
}

Reactor* ReactorPool::getNextWorker()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Reactor* worker = m_workers[m_nextReactor].get();
    m_nextReactor = (m_nextReactor + 1) % m_numWorkers;
    return worker;
}

uint64_t ReactorPool::totalConnections() const
{
    uint64_t total = 0;
    for (const auto& worker : m_workers) {
        total += worker->totalConnections();
    }
    return total;
}