/**
 * @file Selector.cpp
 * @brief Selector实现
 */

#include "Selector.h"
#include <iostream>
#include <cstring>
#include <errno.h>

Selector::Selector()
    : m_maxFd(-1)
{
    // 初始化所有fd_set为0
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
    FD_ZERO(&m_errorSet);
}

Selector::~Selector()
{
    // 不需要关闭fd，它们属于其他对象
}

void Selector::addFd(int fd, EventType event)
{
    if (fd < 0) return;

    m_fds[fd] = event;

    // 根据事件类型添加到对应的fd_set
    if (static_cast<int>(event) & static_cast<int>(EventType::READ)) {
        FD_SET(fd, &m_readSet);
    }
    if (static_cast<int>(event) & static_cast<int>(EventType::WRITE)) {
        FD_SET(fd, &m_writeSet);
    }
    if (static_cast<int>(event) & static_cast<int>(EventType::ERROR)) {
        FD_SET(fd, &m_errorSet);
    }

    updateMaxFd();
}

void Selector::removeFd(int fd)
{
    if (fd < 0) return;

    // 从所有set中移除
    FD_CLR(fd, &m_readSet);
    FD_CLR(fd, &m_writeSet);
    FD_CLR(fd, &m_errorSet);

    m_fds.erase(fd);
    m_readyFds.erase(fd);

    updateMaxFd();
}

void Selector::modifyFd(int fd, EventType event)
{
    if (fd < 0) return;

    // 先移除
    FD_CLR(fd, &m_readSet);
    FD_CLR(fd, &m_writeSet);
    FD_CLR(fd, &m_errorSet);

    // 再添加
    m_fds[fd] = event;

    if (static_cast<int>(event) & static_cast<int>(EventType::READ)) {
        FD_SET(fd, &m_readSet);
    }
    if (static_cast<int>(event) & static_cast<int>(EventType::WRITE)) {
        FD_SET(fd, &m_writeSet);
    }
    if (static_cast<int>(event) & static_cast<int>(EventType::ERROR)) {
        FD_SET(fd, &m_errorSet);
    }

    updateMaxFd();
}

int Selector::wait(int timeoutMs)
{
    // select()会修改fd_set，所以我们使用临时拷贝
    fd_set readSet = m_readSet;
    fd_set writeSet = m_writeSet;
    fd_set errorSet = m_errorSet;

    // timeval结构：秒和微秒
    struct timeval timeout;
    struct timeval* timeoutPtr = nullptr;

    if (timeoutMs >= 0) {
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_usec = (timeoutMs % 1000) * 1000;
        timeoutPtr = &timeout;
    }

    // select() 返回有事件的fd数量
    // nfds = maxFd + 1，因为需要检查0..maxFd范围
    int result = select(m_maxFd + 1, &readSet, &writeSet, &errorSet, timeoutPtr);

    if (result < 0) {
        if (errno == EINTR) {
            // 被信号打断，不是错误
            return 0;
        }
        std::cerr << "[Selector] select() error: " << strerror(errno) << std::endl;
        return -1;
    }

    if (result == 0) {
        // 超时
        return 0;
    }

    // 遍历所有fd，找出有事件的
    m_readyFds.clear();
    for (const auto& pair : m_fds) {
        int fd = pair.first;
        EventType event = EventType::NONE;

        if (FD_ISSET(fd, &readSet)) {
            event = EventType::READ;
        }
        if (FD_ISSET(fd, &writeSet)) {
            event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::WRITE));
        }
        if (FD_ISSET(fd, &errorSet)) {
            event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::ERROR));
        }

        if (event != EventType::NONE) {
            m_readyFds[fd] = event;
        }
    }

    return result;
}

void Selector::updateMaxFd()
{
    m_maxFd = -1;
    for (const auto& pair : m_fds) {
        if (pair.first > m_maxFd) {
            m_maxFd = pair.first;
        }
    }
}
