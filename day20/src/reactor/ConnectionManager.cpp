#include "ConnectionManager.h"
#include "Reactor.h"
#include "log/Logger.h"

ConnectionManager::ConnectionManager(Reactor* reactor, int64_t idleTimeoutMs)
    : m_reactor(reactor)
    , m_idleTimeoutMs(idleTimeoutMs)
    , m_timerManager(TimerType::WHEEL, 1000)  // 1秒tick
    , m_timeoutCount(0)
{
}

ConnectionManager::~ConnectionManager() {
}

// 注册连接：为连接创建定时器，30秒无活动则触发超时
bool ConnectionManager::addConnection(Connection* conn) {
    if (!conn) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    
    // connId
    int64_t timerId = m_timerManager.addTimer(
        m_idleTimeoutMs,
        0,  // 非重复，一次性定时器
        [this, conn](int64_t id) {
            handleTimeout(id);
        }
    );
    
    if (timerId < 0) {
        return false;
    }
    
    conn->setTimerId(timerId);
    m_connections[timerId] = conn;
    m_connectionTimers[conn] = timerId;
    
    return true;
}

// 移除连接：从timer管理中移除（正常关闭时调用）
// 返回true表示从timer管理下移除，返回false表示找不到或已移除
bool ConnectionManager::removeConnection(int connId) {
    Connection* conn = nullptr;
    std::lock_guard<std::mutex> lock(m_mutex);

    auto itConn = m_connections.find(connId);
    if (itConn == m_connections.end()) {
        return false;
    }
    
    conn = itConn->second;

    auto it = m_connectionTimers.find(conn);
    if (it == m_connectionTimers.end()) {
        return false;
    }

    int64_t timerId = it->second;
    m_timerManager.removeTimer(timerId);
    m_connections.erase(timerId);
    m_connectionTimers.erase(it);
    return true;
}

// 更新连接活动：删除旧定时器，创建新定时器（重置超时）
// 这样只有30秒无任何IO活动的连接才会被断开
void ConnectionManager::updateActivity(Connection* conn) {
    if (!conn) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_connectionTimers.find(conn);
    if (it != m_connectionTimers.end()) {
        int64_t oldTimerId = it->second;
        m_timerManager.removeTimer(oldTimerId);
        
        int64_t newTimerId = m_timerManager.addTimer(
            m_idleTimeoutMs,
            0,
            [this, conn](int64_t id) {
                handleTimeout(id);
            }
        );
        
        if (newTimerId >= 0) {
            conn->setTimerId(newTimerId);
            m_connections[newTimerId] = conn;
            m_connections.erase(oldTimerId);
            it->second = newTimerId;
        }
    }
}

size_t ConnectionManager::connectionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connections.size();
}

int64_t ConnectionManager::checkTimeouts() {
    return m_timerManager.tick();
}

int64_t ConnectionManager::getNextCheckTime() const {
    return m_timerManager.getNextExpire();
}

// 超时处理
void ConnectionManager::handleTimeout(int64_t connId) {
    Connection* conn = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_connections.find(connId);
        if (it != m_connections.end()) {
            conn = it->second;
        }
    }
    
    if (conn && m_reactor) {
        m_timeoutCount.fetch_add(1);
        LOG_WARN("Connection timeout: %s (total timeouts: %ld)", 
                 conn->getClientInfo().c_str(), (long)m_timeoutCount.load());
        m_reactor->removeHandler(conn);
    }
}