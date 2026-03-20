/**
 * @file Selector.h
 * @brief epoll()系统调用的封装
 * 
 * ==================== Selector的职责 ====================
 * 
 * Selector封装了Linux epoll I/O多路复用机制，提供统一的接口。
 * 
 * epoll三大系统调用：
 * 1. epoll_create1() - 创建epoll实例
 * 2. epoll_ctl() - 添加/删除/修改监控的fd
 * 3. epoll_wait() - 等待事件发生
 * 
 * ==================== 为什么选择epoll？ ====================
 * 
 * 相比select/poll的优势：
 * - O(1)复杂度：只返回就绪的事件，而非遍历所有fd
 * - 无fd数量限制：只受限于系统最大文件描述符数
 * - 高效的数据结构：使用红黑树管理监控的fd
 * 
 * select的O(n)问题：
 * - 每次调用需要传入fd列表，内核遍历所有fd检查状态
 * - fd数量多时效率急剧下降
 * 
 * ==================== ET vs LT ====================
 * 
 * LT（Level Trigger，水平触发）：
 * - 只要条件满足就一直通知
 * - 简单，但可能多次通知同一事件
 * 
 * ET（Edge Trigger，边缘触发）：
 * - 只在状态变化时通知一次
 * - 高效，但必须一次性处理完所有数据
 * - 配合非阻塞I/O使用
 * 
 * Day05引入ET模式：
 * - 使用EPOLLET标志
 * - 必须配合非阻塞I/O
 * - read/write必须循环直到返回EAGAIN
 * 
 * ==================== Day07的变化 ====================
 * 
 * Selector被Reactor持有，作为其内部组件：
 * - Reactor持有Selector进行事件等待
 * - Reactor根据Selector返回的结果分发事件
 * - 实现了"检测"和"处理"的分离
 */

#ifndef SELECTOR_H
#define SELECTOR_H

#include <sys/epoll.h>
#include <cstddef>
#include <vector>
#include <map>
#include <functional>

/**
 * @brief I/O事件类型枚举
 * 
 * 使用epoll事件常量，但封装为类型安全的枚举类。
 * 提供了|运算符重载，支持组合事件。
 */
enum class EventType {
    NONE = 0,       // 无事件
    READ = EPOLLIN,      // 可读：socket收到数据、监听fd可accept
    WRITE = EPOLLOUT,    // 可写：发送缓冲区有空间
    ERROR = EPOLLERR     // 错误：连接出错、挂起等
};

/**
 * @brief EventType的|运算符重载
 * 
 * 支持组合事件，如：EventType::READ | EventType::WRITE
 * 返回int类型的位掩码，供epoll使用
 */
inline int operator|(EventType a, EventType b) {
    return static_cast<int>(a) | static_cast<int>(b);
}

/**
 * @brief 事件回调函数类型
 * 
 * 可选功能：设置回调函数，在事件发生时自动调用。
 * 当前Reactor实现未使用此回调，而是主动调用Handler方法。
 */
using EventCallback = std::function<void(int fd, EventType)>;

/**
 * @brief Selector类 - I/O多路复用封装
 * 
 * 封装epoll，提供跨平台风格的事件监控接口。
 * 使用ET(边缘触发)模式，必须配合非阻塞I/O。
 * 
 * 使用流程：
 * 1. 构造函数：epoll_create1()创建实例
 * 2. addFd()：添加要监控的fd
 * 3. wait()：阻塞等待事件
 * 4. getReadyFds()：获取就绪的fd列表
 * 5. modifyFd()：修改监控的事件类型
 * 6. removeFd()：停止监控某个fd
 * 
 * 线程安全：
 * - epoll实例本身是线程安全的
 * - 但当前实现是单线程使用，无需额外同步
 */
class Selector {
public:
    /**
     * @brief 构造函数 - 创建epoll实例
     * 
     * 调用epoll_create1(0)创建epoll实例。
     * 参数flags为0表示使用默认标志。
     * 
     * @throw std::runtime_error 如果创建失败
     */
    Selector();

    /**
     * @brief 析构函数 - 关闭epoll实例
     * 
     * 关闭epoll fd，释放内核资源
     */
    ~Selector();

    // 禁用拷贝：epoll实例不能共享
    Selector(const Selector&) = delete;
    Selector& operator=(const Selector&) = delete;

    /**
     * @brief 添加要监控的fd（单事件）
     * @param fd 文件描述符
     * @param event 监控的事件类型
     * 
     * 内部调用epoll_ctl(m_epollFd, EPOLL_CTL_ADD, ...)
     * 将fd添加到epoll的兴趣列表。
     * 
     * 注意：
     * - fd不能重复添加
     * - 默认使用ET模式（EPOLLET）
     */
    void addFd(int fd, EventType event);

    /**
     * @brief 添加要监控的fd（组合事件）
     * @param fd 文件描述符
     * @param events 事件位掩码
     * 
     * 接受多个事件的组合，如：EPOLLIN | EPOLLOUT
     */
    void addFd(int fd, int events);

    /**
     * @brief 移除不再监控的fd
     * @param fd 文件描述符
     * 
     * 内部调用epoll_ctl(m_epollFd, EPOLL_CTL_DEL, ...)
     * 从epoll兴趣列表中删除fd。
     * 
     * 注意：删除后即使fd仍有事件也不会通知
     */
    void removeFd(int fd);

    /**
     * @brief 修改fd的事件（单事件）
     * @param fd 文件描述符
     * @param event 新的事件类型
     * 
     * 内部调用epoll_ctl(m_epollFd, EPOLL_CTL_MOD, ...)
     * 用于动态改变监控的事件类型。
     * 
     * 典型用途：
     * - Connection无数据发送时：interest()返回READ
     * - Connection有数据发送时：interest()返回READ|WRITE
     */
    void modifyFd(int fd, EventType event);

    /**
     * @brief 修改fd的事件（组合事件）
     */
    void modifyFd(int fd, int events);

    /**
     * @brief 等待事件发生
     * @param timeoutMs 超时时间（毫秒），-1表示阻塞等待
     * @return 发生事件的fd数量，0表示超时，-1表示错误
     * 
     * 调用epoll_wait()阻塞等待。
     * 返回后，调用getReadyFds()获取就绪的fd列表。
     * 
     * ET模式特点：
     * - 返回后必须一次性处理完所有数据
     * - 读：循环read直到返回EAGAIN
     * - 写：循环write直到返回EAGAIN
     * 
     * @param timeoutMs 默认-1，表示无限等待
     */
    int wait(int timeoutMs = -1);

    /**
     * @brief 获取有事件发生的fd列表
     * @return map：fd -> 事件类型
     * 
     * 必须在wait()之后调用。
     * 返回所有就绪的fd及其发生的事件类型。
     */
    const std::map<int, EventType>& getReadyFds() const { return m_readyFds; }

    // ===== 回调函数设置（可选功能，当前未使用） =====

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
    int m_epollFd;                              // epoll实例fd
    static const int MAX_EVENTS = 4096;          // 每次最多处理的事件数
    struct epoll_event m_events[MAX_EVENTS];    // 事件数组（内核写入）
    std::map<int, EventType> m_fdEvents;        // fd到监控事件的映射
    std::map<int, EventType> m_readyFds;        // 就绪的fd及其事件

    // 回调函数（可选功能，当前未使用）
    EventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_errorCallback;
};

#endif // SELECTOR_H
