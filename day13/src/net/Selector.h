/**
 * @file Selector.h
 * @brief epoll封装 - I/O多路复用选择器
 */

#ifndef SELECTOR_H
#define SELECTOR_H

#include <sys/epoll.h>
#include <map>
#include <functional>

enum class EventType {
    NONE = 0,
    READ = EPOLLIN,
    WRITE = EPOLLOUT,
    ERROR = EPOLLERR
};

inline int operator|(EventType a, EventType b) {
    return static_cast<int>(a) | static_cast<int>(b);
}

using EventCallback = std::function<void(int fd, EventType)>;

class Selector {
public:
    Selector();
    ~Selector();

    Selector(const Selector&) = delete;
    Selector& operator=(const Selector&) = delete;

    void addFd(int fd, EventType event);
    void addFd(int fd, int events);
    void removeFd(int fd);
    void modifyFd(int fd, EventType event);
    void modifyFd(int fd, int events);
    int wait(int timeoutMs = -1);
    const std::map<int, EventType>& getReadyFds() const { return m_readyFds; }
    size_t getFdCount() const { return m_fdEvents.size(); }

private:
    int m_epollFd;
    static const int MAX_EVENTS = 4096;
    struct epoll_event m_events[MAX_EVENTS];
    std::map<int, EventType> m_fdEvents;
    std::map<int, EventType> m_readyFds;
};

#endif // SELECTOR_H