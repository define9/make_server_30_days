/**
 * @file Selector.h
 * @brief poll()系统调用的封装
 *
 * Selector封装了poll I/O多路复用的核心操作：
 * - pollfd数组管理（无FD_SETSIZE限制）
 * - poll等待
 * - 事件检测
 * 
 * 相比select的优势：
 * - 没有1024 fd的数量限制
 * - 使用动态数组，fd数量可自由扩展
 * - 事件和就绪事件分开，通过revents判断
 */

#ifndef SELECTOR_H
#define SELECTOR_H

#include <poll.h>
#include <cstddef>
#include <vector>
#include <map>
#include <functional>

/**
 * @brief I/O事件类型
 */
enum class EventType {
    NONE = 0,           // 无事件
    READ = POLLIN,      // 可读 (POLLIN)
    WRITE = POLLOUT,    // 可写 (POLLOUT)
    ERROR = POLLERR     // 错误 (POLLERR)
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
 * 使用poll()代替select()，解决FD_SETSIZE限制问题
 *
 * 使用方法：
 * ```cpp
 * Selector selector;
 * selector.setReadCallback([](int fd, EventType) { ... });
 * selector.addFd(5, EventType::READ);
 * selector.wait();  // 阻塞等待事件
 * ```
 */
class Selector {
public:
    /**
     * @brief 构造函数
     */
    Selector();

    /**
     * @brief 析构函数
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
     * @brief 等待事件发生
     * @param timeoutMs 超时时间(毫秒)，-1表示阻塞等待
     * @return 发生事件的fd数量，0表示超时，-1表示错误
     *
     * 调用后，使用getReadyFds()获取有事件的fd
     */
    int wait(int timeoutMs = -1);

    /**
     * @brief 获取有事件发生的fd列表
     * @return fd到事件类型的映射
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
    size_t getFdCount() const { return m_pollfds.size(); }

private:
    /**
     * @brief 通过fd找到pollfd数组中的索引
     */
    int findPollfdIndex(int fd) const;

    std::vector<struct pollfd> m_pollfds;      // pollfd数组
    std::map<int, EventType> m_fdEvents;       // fd到事件的映射
    std::map<int, EventType> m_readyFds;       // 就绪的fd

    EventCallback m_readCallback;   // 读回调
    EventCallback m_writeCallback;  // 写回调
    EventCallback m_errorCallback;  // 错误回调
};

#endif // SELECTOR_H
