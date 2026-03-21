/**
 * @file Reactor.h
 * @brief 反应器 - 事件分发中心
 */

#ifndef REACTOR_H
#define REACTOR_H

#include "net/Selector.h"
#include "net/Connection.h"
#include "EventHandler.h"
#include <map>
#include <atomic>
#include <cstdint>

class Reactor {
public:
    Reactor();
    ~Reactor();

    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    void registerHandler(EventHandler* handler);
    void removeHandler(EventHandler* handler);
    void loop();
    void stop();
    bool hasHandlers() const { return !m_handlers.empty(); }
    size_t handlerCount() const { return m_handlers.size(); }
    void closeAllHandlers();

    uint64_t totalConnections() const { return m_totalConnections; }
    size_t activeConnections() const { return m_handlers.size() - 1; }
    void incrementConnections() { ++m_totalConnections; }

    void setWorkerId(int id) { m_workerId = id; }
    int workerId() const { return m_workerId; }

private:
    void handleEvents();

    Selector m_selector;
    std::map<int, EventHandler*> m_handlers;
    std::atomic<bool> m_running;
    uint64_t m_totalConnections;
    int m_workerId;
};

#endif // REACTOR_H