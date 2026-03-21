/**
 * @file Reactor.cpp
 * @brief Reactor实现
 */

#include "Reactor.h"
#include "EventHandler.h"
#include <iostream>
#include <vector>
#include <errno.h>
#include "Connection.h"

Reactor::Reactor()
    : m_running(false)
    , m_totalConnections(0)
    , m_workerId(-1)
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
        std::cout << "[Reactor] removeHandler: removing Connection fd=" << fd << std::endl;
        delete conn;
    }
}

void Reactor::loop()
{
    m_running = true;

    while (m_running) {
        int numEvents = m_selector.wait(1000);

        if (numEvents < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[Reactor] epoll_wait() error" << std::endl;
            break;
        }

        if (numEvents == 0) {
            continue;
        }

        handleEvents();
    }

    closeAllHandlers();
}

void Reactor::stop()
{
    m_running = false;
}

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
        }

        if (static_cast<int>(event) & static_cast<int>(EventType::WRITE)) {
            if (!handler->handleWrite()) {
                toRemove.push_back(handler);
                continue;
            }
            if (!handler->hasDataToSend()) {
                m_selector.modifyFd(fd, EventType::READ);
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