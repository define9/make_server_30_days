/**
 * @file Selector.cpp
 * @brief Selector实现 - poll版本
 */

#include "Selector.h"
#include <iostream>
#include <cstring>
#include <errno.h>

Selector::Selector()
{
}

Selector::~Selector()
{
}

int Selector::findPollfdIndex(int fd) const
{
    for (size_t i = 0; i < m_pollfds.size(); ++i) {
        if (m_pollfds[i].fd == fd) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Selector::addFd(int fd, EventType event)
{
    if (fd < 0) return;

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = static_cast<short>(event);
    pfd.revents = 0;

    m_pollfds.push_back(pfd);
    m_fdEvents[fd] = event;
}

void Selector::removeFd(int fd)
{
    if (fd < 0) return;

    int index = findPollfdIndex(fd);
    if (index >= 0) {
        m_pollfds.erase(m_pollfds.begin() + index);
    }

    m_fdEvents.erase(fd);
    m_readyFds.erase(fd);
}

void Selector::modifyFd(int fd, EventType event)
{
    if (fd < 0) return;

    int index = findPollfdIndex(fd);
    if (index >= 0) {
        m_pollfds[index].events = static_cast<short>(event);
        m_fdEvents[fd] = event;
    }
}

int Selector::wait(int timeoutMs)
{
    // poll()不会修改events字段，只修改revents
    // 所以我们不需要复制副本

    // timeout为-1时表示阻塞等待，poll用-1表示无限等待
    int timeout = (timeoutMs < 0) ? -1 : timeoutMs;

    int result = poll(m_pollfds.data(), m_pollfds.size(), timeout);

    if (result < 0) {
        if (errno == EINTR) {
            return 0;
        }
        std::cerr << "[Selector] poll() error: " << strerror(errno) << std::endl;
        return -1;
    }

    if (result == 0) {
        return 0;
    }

    // 遍历pollfd数组，找出有事件的
    m_readyFds.clear();
    for (const auto& pfd : m_pollfds) {
        if (pfd.revents != 0) {
            EventType event = EventType::NONE;

            if (pfd.revents & POLLIN) {
                event = EventType::READ;
            }
            if (pfd.revents & POLLOUT) {
                event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::WRITE));
            }
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::ERROR));
            }

            if (event != EventType::NONE) {
                m_readyFds[pfd.fd] = event;
            }
        }
    }

    return result;
}
