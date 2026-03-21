/**
 * @file ConnectionManager.h
 * @brief 连接管理器 - 管理所有连接的生命周期和超时
 */

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "net/Connection.h"
#include "timer/TimerManager.h"
#include <map>
#include <mutex>
#include <atomic>
#include <cstdint>

class Reactor;

/**
 * @brief 连接管理器 - 负责连接的生命周期管理和超时检测
 * 
 * ==================== 连接超时管理架构 ====================
 * 
 *     ConnectionManager
 *          │
 *          ├── 管理所有 Connection 对象
 *          │
 *          └── TimerManager ──定时检查──> 检查连接空闲时间
 *                                       │
 *                                       ▼
 *                                  断开超时连接
 * 
 * ==================== 核心功能 ====================
 * 
 * 1. 连接注册/注销
 * 2. 空闲超时检测
 * 3. 定时器管理
 * 4. 统计信息
 */
class ConnectionManager {
public:
    /**
     * @brief 构造函数
     * @param reactor 关联的Reactor，用于关闭连接
     * @param idleTimeoutMs 空闲超时时间（毫秒），默认30000ms（30秒）
     */
    ConnectionManager(Reactor* reactor, int64_t idleTimeoutMs = 30000);
    
    ~ConnectionManager();

    // 禁用拷贝
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    /**
     * @brief 注册新连接
     * @param conn 要注册的连接
     * @return true注册成功，false失败
     */
    bool addConnection(Connection* conn);

    /**
     * @brief 移除连接
     * @param connId 要移除的连接Id
     * @return true 如果连接在timer管理下被移除（尚未超时），false 如果已超时
     */
    bool removeConnection(int connId);

    /**
     * @brief 更新连接活动时间（每次有IO操作时调用）
     * @param conn 要更新的连接
     */
    void updateActivity(Connection* conn);

    /**
     * @brief 获取连接数
     */
    size_t connectionCount() const;

    /**
     * @brief 获取超时断开数
     */
    uint64_t timeoutCount() const { return m_timeoutCount.load(); }

    /**
     * @brief 获取空闲超时时间
     */
    int64_t idleTimeout() const { return m_idleTimeoutMs; }

    /**
     * @brief 检查所有连接是否超时
     * @return 实际等待时间
     */
    int64_t checkTimeouts();

    /**
     * @brief 获取下一个超时检查时间
     */
    int64_t getNextCheckTime() const;

private:
    /**
     * @brief 处理连接超时
     * @param connId 连接ID（timer id）
     */
    void handleTimeout(int64_t connId);

    Reactor* m_reactor;
    int64_t m_idleTimeoutMs;
    
    // 连接映射：timerId -> Connection*
    std::map<int64_t, Connection*> m_connections;
    // 反向映射：Connection* -> timerId
    std::map<Connection*, int64_t> m_connectionTimers;
    
    mutable std::mutex m_mutex;
    TimerManager m_timerManager;
    std::atomic<uint64_t> m_timeoutCount;
};

#endif // CONNECTION_MANAGER_H