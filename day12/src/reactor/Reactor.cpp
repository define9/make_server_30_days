/**
 * @file Reactor.cpp
 * @brief Reactor实现
 */

#include "Reactor.h"
#include "EventHandler.h"
#include "net/Connection.h"
#include <iostream>
#include <vector>
#include <errno.h>

Reactor::Reactor()
    : m_running(false)
    , m_totalConnections(0)
    , m_workerId(-1)
    , m_connManager(this, 30000)  // 30秒空闲超时
{
}

Reactor::~Reactor()
{
    stop();
}

void Reactor::registerHandler(EventHandler* handler)
{
    if (!handler) return;

    int fd = handler->fd();
    m_handlers[fd] = handler;
    m_selector.addFd(fd, handler->interest());
    
    // Connection类型需要注册到连接管理器以支持超时检测
    Connection* conn = dynamic_cast<Connection*>(handler);
    if (conn) {
        m_connManager.addConnection(conn);
    }
}

void Reactor::removeHandler(EventHandler* handler)
{
    if (!handler) return;

    // 从 handlers 里移除
    int fd = handler->fd();

    m_handlers.erase(fd);
    m_selector.removeFd(fd);

    // 关闭
    handler->handleClose();

    // Connection类型需要从连接管理器移除
    Connection* conn = dynamic_cast<Connection*>(handler);
    if (conn) {
        std::cout << "[Reactor] removeHandler: removing Connection fd=" << fd << ", timerId=" << conn->timerId() << std::endl;
        m_connManager.removeConnection(conn->timerId());
        delete conn;
    }
}

void Reactor::loop()
{
    m_running.store(true);

    while (m_running.load()) {
        // 根据下一个超时时间设置epoll_wait超时
        int64_t nextTimeout = m_connManager.getNextCheckTime();
        int waitTime = (nextTimeout > 0 && nextTimeout < 1000) ? nextTimeout : 1000;
        
        int numEvents = m_selector.wait(waitTime);

        if (numEvents < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[Reactor] epoll_wait() error" << std::endl;
            break;
        }

        if (numEvents > 0) {
            handleEvents();
        }

        // 每次循环都检查超时（定时器tick）
        m_connManager.checkTimeouts();
    }

    closeAllHandlers();
}

void Reactor::stop()
{
    m_running.store(false);
}

// 关闭所有handler：统一管理Connection的删除
void Reactor::closeAllHandlers()
{
    if (m_workerId >= 0) {
        std::cout << "[Reactor] Closing all handlers for Worker " << m_workerId << "..." << std::endl;
    }
    
    while (!m_handlers.empty()) {
        auto it = m_handlers.begin();
        EventHandler* handler = it->second;
        if (handler) {
            removeHandler(handler);
        }
    }
}

// 处理IO事件：读/写/错误
// 读写成功时更新连接活动时间（重置超时定时器）
void Reactor::handleEvents()
{
    const auto& readyFds = m_selector.getReadyFds();
    std::vector<EventHandler*> toRemove;

    for (const auto& pair : readyFds) {
        int fd = pair.first;
        EventType event = pair.second;

        auto it = m_handlers.find(fd);
        if (it == m_handlers.end()) {
            continue;
        }
        EventHandler* handler = it->second;

        if (static_cast<int>(event) & static_cast<int>(EventType::READ)) {
            if (!handler->handleRead()) {
                toRemove.push_back(handler);
                continue;
            }
            if (handler->hasDataToSend()) {
                m_selector.modifyFd(fd, handler->interest());
            }
            // 读成功，重置超时定时器
            Connection* conn = dynamic_cast<Connection*>(handler);
            if (conn) {
                m_connManager.updateActivity(conn);
            }
        }

        if (static_cast<int>(event) & static_cast<int>(EventType::WRITE)) {
            if (!handler->handleWrite()) {
                toRemove.push_back(handler);
                continue;
            }
            if (!handler->hasDataToSend()) {
                m_selector.modifyFd(fd, EventType::READ);
            }
            // 写成功，重置超时定时器
            Connection* conn = dynamic_cast<Connection*>(handler);
            if (conn) {
                m_connManager.updateActivity(conn);
            }
        }

        if (static_cast<int>(event) & static_cast<int>(EventType::ERROR)) {
            std::cerr << "[Reactor] Error on fd " << fd << std::endl;
            toRemove.push_back(handler);
        }
    }

    for (EventHandler* handler : toRemove) {
        removeHandler(handler);
    }
}