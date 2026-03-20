/**
 * @file Selector.h
 * @brief epoll()系统调用的封装
 *
 * Selector封装了epoll I/O多路复用的核心操作：
 * - epoll_create创建epoll实例
 * - epoll_ctl管理监控的fd
 * - epoll_wait等待事件
 * 
 * Day05变化：
 * - 使用ET(边缘触发)模式
 * - 配合非阻塞I/O，必须循环处理所有数据
 */

#ifndef SELECTOR_H
#define SELECTOR_H

#include <sys/epoll.h>
#include <cstddef>
#include <vector>
#include <map>
#include <functional>

/**
 * @brief I/O事件类型 - 使用epoll事件常量
 */
enum class EventType {
    NONE = 0,
    READ = EPOLLIN,      // 可读
    WRITE = EPOLLOUT,    // 可写
    ERROR = EPOLLERR     // 错误
};

inline int operator|(EventType a, EventType b) {
    return static_cast<int>(a) | static_cast<int>(b);
}

/**
 * @brief 事件回调函数类型
 */
using EventCallback = std::function<void(int fd, EventType)>;

/**
 * @brief Selector类 - I/O多路复用封装
 *
 * 使用epoll()实现O(1)事件通知
 * Day05使用ET(边缘触发)模式，必须配合非阻塞I/O
 */
class Selector {
public:
    /**
     * @brief 构造函数 - 创建epoll实例
     */
    Selector();

    /**
     * @brief 析构函数 - 关闭epoll实例
     */
    ~Selector();

    // 禁用拷贝
    Selector(const Selector&) = delete;
    Selector& operator=(const Selector&) = delete;

    /**
     * @brief 添加要监控的fd
     * @param fd 文件描述符
     * @param event 监控的事件类型
     */
    void addFd(int fd, EventType event);

    /**
     * @brief 移除不再监控的fd
     * @param fd 文件描述符
     */
    void removeFd(int fd);

    /**
     * @brief 修改fd的事件
     * @param fd 文件描述符
     * @param event 新的事件类型
     */
    void modifyFd(int fd, EventType event);

    /**
     * @brief 修改fd的事件（接受组合事件）
     */
    void modifyFd(int fd, int events);

    /**
     * @brief 等待事件发生
     * @param timeoutMs 超时时间(毫秒)，-1表示阻塞等待
     * @return 发生事件的fd数量，0表示超时，-1表示错误
     */
    int wait(int timeoutMs = -1);

    /**
     * @brief 获取有事件发生的fd列表
     */
    const std::map<int, EventType>& getReadyFds() const { return m_readyFds; }

    /**
     * @brief 设置读事件回调
     */
    void setReadCallback(EventCallback cb) { m_readCallback = std::move(cb); }

    /**
     * @brief 设置写事件回调
     */
    void setWriteCallback(EventCallback cb) { m_writeCallback = std::move(cb); }

    /**
     * @brief 设置错误回调
     */
    void setErrorCallback(EventCallback cb) { m_errorCallback = std::move(cb); }

    /**
     * @brief 获取当前监控的fd数量
     */
    size_t getFdCount() const { return m_fdEvents.size(); }

private:
    int m_epollFd;                              // epoll文件描述符
    static const int MAX_EVENTS = 4096;          // 每次最多处理的事件数
    struct epoll_event m_events[MAX_EVENTS];    // 事件数组
    std::map<int, EventType> m_fdEvents;        // fd到事件的映射
    std::map<int, EventType> m_readyFds;        // 就绪的fd

    EventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_errorCallback;
};

#endif // SELECTOR_H
