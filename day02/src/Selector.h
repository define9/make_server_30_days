/**
 * @file Selector.h
 * @brief select()系统调用的封装
 *
 * Selector封装了select I/O多路复用的核心操作：
 * - fd_set管理
 * - select等待
 * - 事件检测
 */

#ifndef SELECTOR_H
#define SELECTOR_H

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <functional>

/**
 * @brief I/O事件类型
 */
enum class EventType {
    NONE = 0,    // 无事件
    READ = 1,     // 可读
    WRITE = 2,    // 可写
    ERROR = 4     // 错误
};

/**
 * @brief 事件回调函数类型
 */
using EventCallback = std::function<void(int fd, EventType)>;

/**
 * @brief Selector类 - I/O多路复用封装
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
     * @brief 获取当前监控的最大fd
     */
    int getMaxFd() const { return m_maxFd; }

private:
    /**
     * @brief 更新maxFd
     */
    void updateMaxFd();

    fd_set m_readSet;              // 读事件集合
    fd_set m_writeSet;             // 写事件集合
    fd_set m_errorSet;             // 错误事件集合

    std::map<int, EventType> m_fds;    // fd到事件的映射
    std::map<int, EventType> m_readyFds; // 就绪的fd

    int m_maxFd;                    // 当前最大fd
    EventCallback m_readCallback;   // 读回调
    EventCallback m_writeCallback;  // 写回调
    EventCallback m_errorCallback;  // 错误回调
};

#endif // SELECTOR_H
