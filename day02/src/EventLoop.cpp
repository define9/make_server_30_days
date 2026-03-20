/**
 * @file EventLoop.cpp
 * @brief EventLoop实现
 */

#include "EventLoop.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <errno.h>

EventLoop::EventLoop()
    : m_running(false)
    , m_serverFd(-1)
{
}

EventLoop::~EventLoop()
{
    stop();
}

void EventLoop::loop(int serverFd)
{
    m_serverFd = serverFd;
    m_selector.addFd(serverFd, EventType::READ);

    m_running = true;
    std::cout << "[EventLoop] Started" << std::endl;

    while (m_running) {
        // 等待事件发生
        int numEvents = m_selector.wait(1000);  // 1秒超时

        if (numEvents < 0) {
            if (errno == EINTR) {
                continue;  // 被信号打断
            }
            std::cerr << "[EventLoop] select() error" << std::endl;
            break;
        }

        if (numEvents == 0) {
            continue;  // 超时，继续循环
        }

        // 处理事件
        handleEvents(serverFd);
    }

    std::cout << "[EventLoop] Stopped" << std::endl;
}

void EventLoop::stop()
{
    if (m_serverFd >= 0) {
        m_selector.removeFd(m_serverFd);
        m_serverFd = -1;
    }
    m_running = false;
}

void EventLoop::handleEvents(int serverFd)
{
    const auto& readyFds = m_selector.getReadyFds();
    std::vector<int> toRemove;

    for (const auto& pair : readyFds) {
        int fd = pair.first;
        EventType event = pair.second;

        // 服务器socket有事件 -> 新连接
        if (fd == serverFd && static_cast<int>(event) & static_cast<int>(EventType::READ)) {
            std::cout << "[EventLoop] Server socket ready for accept" << std::endl;
            if (m_newConnectionCallback) {
                m_newConnectionCallback(serverFd);
            }
            continue;
        }

        // 客户端socket有事件
        auto it = m_connections.find(fd);
        if (it == m_connections.end()) {
            continue;
        }
        Connection* conn = it->second;

        if (static_cast<int>(event) & static_cast<int>(EventType::READ)) {
            if (!conn->handleRead()) {
                toRemove.push_back(fd);
                continue;
            }
        }

        if (static_cast<int>(event) & static_cast<int>(EventType::WRITE)) {
            if (!conn->handleWrite()) {
                toRemove.push_back(fd);
                continue;
            }
        }

        if (static_cast<int>(event) & static_cast<int>(EventType::ERROR)) {
            std::cerr << "[EventLoop] Error on fd " << fd << std::endl;
            toRemove.push_back(fd);
        }
    }

    for (int fd : toRemove) {
        removeConnection(fd);
    }
}

void EventLoop::addConnection(int clientSocket, const sockaddr_in& clientAddr)
{
    Connection* conn = new Connection(clientSocket, clientAddr);
    m_connections[clientSocket] = conn;
    m_selector.addFd(clientSocket, EventType::READ);
    std::cout << "[EventLoop] Added connection fd=" << clientSocket << std::endl;
}

void EventLoop::removeConnection(int fd)
{
    auto it = m_connections.find(fd);
    if (it != m_connections.end()) {
        delete it->second;
        m_connections.erase(it);
    }
    m_selector.removeFd(fd);
    std::cout << "[EventLoop] Removed connection fd=" << fd << std::endl;
}
