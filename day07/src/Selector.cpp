/**
 * @file Selector.cpp
 * @brief Selector实现 - epoll ET版本
 * 
 * 本文件实现对Linux epoll的封装。
 * 使用ET(边缘触发)模式，配合非阻塞I/O工作。
 */

#include "Selector.h"
#include <iostream>
#include <cstring>
#include <errno.h>
#include <unistd.h>

/**
 * @brief 构造函数 - 创建epoll实例
 * 
 * epoll_create1(0)创建一个新的epoll实例：
 * - 返回一个文件描述符作为epoll实例的句柄
 * - flags=0表示使用默认设置
 * 
 * 为什么用epoll_create1而不是epoll_create？
 * - epoll_create已废弃
 * - epoll_create1可以传入flags参数
 * - EPOLL_CLOEXEC标志可在fork后自动关闭，但这里不需要
 */
Selector::Selector()
    : m_epollFd(-1)
{
    m_epollFd = epoll_create1(0);
    if (m_epollFd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

/**
 * @brief 析构函数 - 关闭epoll实例
 * 
 * 关闭epoll fd，释放内核资源
 */
Selector::~Selector()
{
    if (m_epollFd >= 0) {
        close(m_epollFd);
    }
}

/**
 * @brief 添加要监控的fd（单事件版本）
 * 
 * epoll_ctl ADD操作：
 * - 将fd添加到epoll的兴趣列表
 * - 指定关心的事件类型
 * - 自动启用ET模式（EPOLLET）
 * 
 * @param fd 要监控的文件描述符
 * @param event 关心的事件类型
 */
void Selector::addFd(int fd, EventType event)
{
    if (fd < 0) return;

    struct epoll_event ev;
    // 设置事件类型 + 启用ET模式
    ev.events = static_cast<uint32_t>(event) | EPOLLET;
    ev.data.fd = fd;

    // 添加到epoll兴趣列表
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "[Selector] epoll_ctl ADD failed: " << strerror(errno) << std::endl;
        return;
    }

    // 记录到本地映射（用于追踪）
    m_fdEvents[fd] = event;
}

/**
 * @brief 添加要监控的fd（组合事件版本）
 * 
 * 支持传入事件位掩码的版本，如 EPOLLIN | EPOLLOUT
 * 
 * @param fd 文件描述符
 * @param events 事件位掩码
 */
void Selector::addFd(int fd, int events)
{
    if (fd < 0) return;

    struct epoll_event ev;
    ev.events = static_cast<uint32_t>(events) | EPOLLET;
    ev.data.fd = fd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "[Selector] epoll_ctl ADD failed: " << strerror(errno) << std::endl;
        return;
    }

    m_fdEvents[fd] = EventType::READ;
}

/**
 * @brief 移除不再监控的fd
 * 
 * epoll_ctl DEL操作：
 * - 从epoll兴趣列表中移除fd
 * - 移除后不再收到该fd的事件通知
 * 
 * @param fd 要移除的文件描述符
 */
void Selector::removeFd(int fd)
{
    if (fd < 0) return;

    // EPOLL_CTL_DEL不需要传入事件，第三个参数可为nullptr
    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        // EPOLL_CTL_DEL失败通常是因为fd已关闭，无需处理
        std::cerr << "[Selector] epoll_ctl DEL failed: " << strerror(errno) << std::endl;
    }

    // 从本地映射中删除
    m_fdEvents.erase(fd);
    m_readyFds.erase(fd);
}

/**
 * @brief 修改fd的事件（单事件版本）
 * 
 * epoll_ctl MOD操作：
 * - 修改已注册fd的事件类型
 * - 用于动态调整监控策略
 * 
 * 典型场景：
 * - Connection无数据时：只监控READ
 * - Connection有数据时：同时监控READ和WRITE
 * 
 * @param fd 文件描述符
 * @param event 新的事件类型
 */
void Selector::modifyFd(int fd, EventType event)
{
    if (fd < 0) return;

    struct epoll_event ev;
    ev.events = static_cast<uint32_t>(event) | EPOLLET;
    ev.data.fd = fd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "[Selector] epoll_ctl MOD failed: " << strerror(errno) << std::endl;
        return;
    }

    m_fdEvents[fd] = event;
}

/**
 * @brief 修改fd的事件（组合事件版本）
 */
void Selector::modifyFd(int fd, int events)
{
    if (fd < 0) return;

    struct epoll_event ev;
    ev.events = static_cast<uint32_t>(events) | EPOLLET;
    ev.data.fd = fd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "[Selector] epoll_ctl MOD failed: " << strerror(errno) << std::endl;
        return;
    }

    m_fdEvents[fd] = EventType::READ;
}

/**
 * @brief 等待事件发生
 * 
 * epoll_wait是阻塞调用：
 * - 等待直到有事件发生或超时
 * - 将就绪的事件写入m_events数组
 * 
 * ET模式注意事项：
 * - 必须循环处理所有就绪事件
 * - read/write需要循环直到返回EAGAIN
 * - 否则可能丢失事件
 * 
 * @param timeoutMs 超时时间（毫秒）
 *        -1: 无限等待
 *        0: 立即返回
 *        >0: 等待指定毫秒
 * @return 
 *        >0: 发生事件的数量
 *        0: 超时，无事件
 *        -1: 错误（见errno）
 */
int Selector::wait(int timeoutMs)
{
    // 处理timeout参数
    int timeout = (timeoutMs < 0) ? -1 : timeoutMs;

    // 调用epoll_wait等待事件
    // m_events数组由内核填充，就绪事件的fd和events
    int numEvents = epoll_wait(m_epollFd, m_events, MAX_EVENTS, timeout);

    // 错误处理
    if (numEvents < 0) {
        if (errno == EINTR) {
            // EINTR：被信号中断，不是错误，下次继续等待
            return 0;
        }
        std::cerr << "[Selector] epoll_wait() error: " << strerror(errno) << std::endl;
        return -1;
    }

    // 超时：无事件发生，继续
    if (numEvents == 0) {
        return 0;
    }

    // 解析就绪事件，填充m_readyFds
    m_readyFds.clear();
    for (int i = 0; i < numEvents; ++i) {
        int fd = m_events[i].data.fd;
        uint32_t events = m_events[i].events;

        EventType event = EventType::NONE;

        // EPOLLIN → READ
        if (events & EPOLLIN) {
            event = EventType::READ;
        }
        // EPOLLOUT → WRITE（使用|合并）
        if (events & EPOLLOUT) {
            event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::WRITE));
        }
        // EPOLLERR/EPOLLHUP → ERROR
        if (events & (EPOLLERR | EPOLLHUP)) {
            event = static_cast<EventType>(static_cast<int>(event) | static_cast<int>(EventType::ERROR));
        }

        // 只记录有意义的事件
        if (event != EventType::NONE) {
            m_readyFds[fd] = event;
        }
    }

    return numEvents;
}
