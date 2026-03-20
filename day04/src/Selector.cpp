/**
 * @file Selector.cpp
 * @brief Selector实现 - epoll版本
 *
 * 使用epoll实现I/O多路复用，支持LT(水平触发)模式
 */

#include "Selector.h"
#include <iostream>
#include <cstring>
#include <errno.h>
#include <unistd.h>

Selector::Selector()
    : m_epollFd(-1)
{
    m_epollFd = epoll_create1(0);
    if (m_epollFd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

Selector::~Selector()
{
    if (m_epollFd >= 0) {
        close(m_epollFd);
    }
}

void Selector::addFd(int fd, EventType event)
{
    if (fd < 0) return;

    struct epoll_event ev;
    ev.events = static_cast<uint32_t>(event);
    ev.data.fd = fd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "[Selector] epoll_ctl ADD failed: " << strerror(errno) << std::endl;
        return;
    }

    m_fdEvents[fd] = event;
}

void Selector::removeFd(int fd)
{
    if (fd < 0) return;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        std::cerr << "[Selector] epoll_ctl DEL failed: " << strerror(errno) << std::endl;
    }

    m_fdEvents.erase(fd);
    m_readyFds.erase(fd);
}

void Selector::modifyFd(int fd, EventType event)
{
    if (fd < 0) return;

    struct epoll_event ev;
    ev.events = static_cast<uint32_t>(event);
    ev.data.fd = fd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "[Selector] epoll_ctl MOD failed: " << strerror(errno) << std::endl;
        return;
    }

    m_fdEvents[fd] = event;
}

int Selector::wait(int timeoutMs)
{
    int timeout = (timeoutMs < 0) ? -1 : timeoutMs;

    int numEvents = epoll_wait(m_epollFd, m_events, MAX_EVENTS, timeout);

    if (numEvents < 0) {
        if (errno == EINTR) {
            return 0;
        }
        std::cerr << "[Selector] epoll_wait() error: " << strerror(errno) << std::endl;
        return -1;
    }

    if (numEvents == 0) {
        return 0;
    }

    m_readyFds.clear();
    for (int i = 0; i < numEvents; ++i) {
        int fd = m_events[i].data.fd;
        uint32_t events = m_events[i].events;

        EventType event = EventType::NONE;

        if (events & EPOLLIN) {
            event = EventType::READ;
        }
        if (events & EPOLLOUT) {
            event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::WRITE));
        }
        if (events & (EPOLLERR | EPOLLHUP)) {
            event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::ERROR));
        }

        if (event != EventType::NONE) {
            m_readyFds[fd] = event;
        }
    }

    return numEvents;
}
