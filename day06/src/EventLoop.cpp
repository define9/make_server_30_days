/**
 * @file EventLoop.cpp
 * @brief EventLoop实现 - Day06 ET + 非阻塞版本
 *
 * == Day06核心变化 ==
 *
 * 相比Day05：
 * - Server socket也设置为O_NONBLOCK
 * - acceptAllConnections()循环接受直到EAGAIN
 * - 这确保了多连接时不会遗漏
 *
 * == ET模式核心机制 ==
 *
 * 1. 初始只注册READ事件：客户端连接创建时只监听可读
 * 2. 有数据时动态注册WRITE：收到数据后，如果有数据要回复，注册WRITE事件
 * 3. 发完取消WRITE：数据全部发送完毕后，取消WRITE监听
 *
 * 为什么需要动态注册？
 * - LT模式：只要buffer不满，WRITE一直触发 → 空转浪费CPU
 * - ET模式：只在"变为可写"时通知一次，必须自己管理注册/取消
 *
 * 典型流程：
 *   客户端发送"hello"
 *     → epoll返回READ事件
 *     → handleRead()把数据放入m_writeBuffer
 *     → modifyFd(READ|WRITE) 注册WRITE
 *     → 下一轮epoll返回WRITE事件
 *     → handleWrite()发送数据
 *     → 发完了modifyFd(READ) 取消WRITE
 */

#include "EventLoop.h"
#include <iostream>
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
    std::cout << "[EventLoop] Started (ET mode)" << std::endl;

    while (m_running) {
        int numEvents = m_selector.wait(1000);

        if (numEvents < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[EventLoop] epoll_wait() error" << std::endl;
            break;
        }

        if (numEvents == 0) {
            continue;
        }

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
    // 延迟删除：不能在遍历中直接删除map元素，会导致迭代器失效
    std::vector<int> toRemove;

    for (const auto& pair : readyFds) {
        int fd = pair.first;
        EventType event = pair.second;

        // 服务器socket触发READ → 新客户端连接到达
        if (fd == serverFd && static_cast<int>(event) & static_cast<int>(EventType::READ)) {
            std::cout << "[EventLoop] Server socket ready for accept" << std::endl;
            if (m_newConnectionCallback) {
                m_newConnectionCallback(serverFd);
            }
            continue;
        }

        // 查找对应的Connection
        auto it = m_connections.find(fd);
        if (it == m_connections.end()) {
            continue;
        }
        Connection* conn = it->second;

        // ========== READ 事件处理 ==========
        // ET模式：只要buffer有数据就触发，但只通知一次，所以要循环读
        if (static_cast<int>(event) & static_cast<int>(EventType::READ)) {
            if (!conn->handleRead()) {
                toRemove.push_back(fd);
                continue;
            }
            // 关键：读完后检查是否有数据要回复(echo)
            // 如果有数据要发送，动态注册WRITE事件
            // 这样下一轮epoll_wait就会返回WRITE事件（当socket可写时）
            // 但如果一直注册write的话，这个事件就会一直返回
            if (conn->hasDataToSend()) {
                m_selector.modifyFd(fd, EventType::READ | EventType::WRITE);
            }
        }

        // ========== WRITE 事件处理 ==========
        // ET模式：只在"变为可写"时通知一次
        // 当发送缓冲区从满变为不满时，才触发WRITE
        if (static_cast<int>(event) & static_cast<int>(EventType::WRITE)) {
            if (!conn->handleWrite()) {
                toRemove.push_back(fd);
                continue;
            }
            // 数据全部发送完毕后，取消WRITE监听
            // 原因：ET模式下，如果没有数据要发送，不需要收到WRITE通知
            //      否则会空等（socket一直可写但没数据要发）
            if (!conn->hasDataToSend()) {
                m_selector.modifyFd(fd, EventType::READ);
            }
        }

        // ========== ERROR 事件处理 ==========
        // EPOLLERR | EPOLLHUP 始终被监控，不需要单独注册
        if (static_cast<int>(event) & static_cast<int>(EventType::ERROR)) {
            std::cerr << "[EventLoop] Error on fd " << fd << std::endl;
            toRemove.push_back(fd);
        }
    }

    // 统一删除：避免在迭代中修改map
    for (int fd : toRemove) {
        removeConnection(fd);
    }
}

void EventLoop::addConnection(int clientSocket, const sockaddr_in& clientAddr)
{
    Connection* conn = new Connection(clientSocket, clientAddr);
    m_connections[clientSocket] = conn;
    // 初始只注册READ事件：等待客户端发送数据
    // WRITE事件会在handleRead后，根据hasDataToSend动态添加
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
