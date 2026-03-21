/**
 * @file ReactorPool.h
 * @brief Reactor线程池 - 管理主Reactor和Worker Reactor池
 */

#ifndef REACTOR_POOL_H
#define REACTOR_POOL_H

#include "Reactor.h"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

class Server;

class ReactorPool {
public:
    ReactorPool(uint16_t port, int numWorkers);
    ~ReactorPool();

    ReactorPool(const ReactorPool&) = delete;
    ReactorPool& operator=(const ReactorPool&) = delete;

    void start();
    void stop();
    void waitForExit();
    Reactor* getNextWorker();
    uint64_t totalConnections() const;

private:
    std::unique_ptr<Reactor> m_mainReactor;
    std::vector<std::unique_ptr<Reactor>> m_workers;
    std::vector<std::thread> m_threads;
    std::unique_ptr<Server> m_server;
    
    int m_numWorkers;
    int m_nextReactor;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_running;
};

#endif // REACTOR_POOL_H