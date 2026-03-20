/**
 * @file Reactor.h
 * @brief 反应器类 - Reactor模式核心
 * 
 * ==================== Reactor模式中的Reactor ====================
 * 
 * Reactor是整个事件驱动架构的调度中心，相当于"事件中枢神经"。
 * 
 * 核心职责：
 * 1. 事件循环（Event Loop）
 *    - 阻塞等待I/O多路复用器（Selector/epoll）返回就绪事件
 *    - 永不退出（直到stop()被调用）
 * 
 * 2. 事件分发（Event Dispatching）
 *    - 根据就绪的fd找到对应的Handler
 *    - 调用Handler的handleRead/handleWrite/handleClose方法
 * 
 * 3. Handler生命周期管理
 *    - 注册新的Handler（当Server accept新连接时）
 *    - 移除Handler（当连接断开或出错时）
 * 
 * ==================== 事件流 ====================
 * 
 *   [TCP数据包到达内核缓冲区]
 *            ↓
 *   [epoll_wait返回就绪事件]
 *            ↓
 *   [Reactor.handleEvents()]
 *            ↓
 *   [根据fd找到Handler]
 *            ↓
 *   [调用handler->handleRead/handleWrite/handleClose]
 * 
 * ==================== Day07核心变化 ====================
 * 
 * - Reactor是事件分发中心
 * - 管理所有EventHandler的注册
 * - 根据事件类型分发到对应的Handler
 * - 从"主动调用"模式变为"被动回调"模式
 * 
 * ==================== 线程安全说明 ====================
 * 
 * 当前实现是单线程Reactor：
 * - 所有事件在主线程中处理
 * - 优点：无需加锁，编程简单
 * - 缺点：无法利用多核CPU
 * 
 * Day09会介绍多线程Reactor：
 * - 主Reactor负责监听（accept）
 * - 子Reactor负责处理连接
 * - 需要引入锁保护共享状态
 */

#ifndef REACTOR_H
#define REACTOR_H

#include "Selector.h"
#include "EventHandler.h"
#include <map>
#include <functional>

class EventHandler;

/**
 * @brief 反应器类 - Reactor模式的核心
 * 
 * Reactor是整个事件驱动服务器的"引擎"。
 * 它封装了事件循环，负责：
 * - 持有Selector进行I/O多路复用
 * - 管理所有注册的EventHandler（通过fd索引）
 * - 事件循环：等待事件 → 分发到Handler
 * 
 * 设计要点：
 * - 使用std::map<int, EventHandler*>管理fd到Handler的映射
 * - fd作为唯一标识符，保证O(log n)查找效率
 * - 禁用拷贝，确保只有一个Reactor拥有所有Handler
 */
class Reactor {
public:
    /**
     * @brief 构造函数
     * 
     * 初始化Selector和运行标志
     */
    Reactor();
    
    /**
     * @brief 析构函数
     * 
     * 自动停止事件循环，清理资源
     */
    ~Reactor();

    // 禁用拷贝：Reactor拥有Handler资源，不能共享
    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    /**
     * @brief 注册Handler到Reactor
     * @param handler 要注册的事件处理器（不能为nullptr）
     * 
     * 注册流程：
     * 1. 将handler添加到m_handlers映射表
     * 2. 调用selector.addFd()开始监控该fd
     * 
     * 注意：
     * - handler的fd成为Reactor管理的唯一键
     * - 同一个fd不能重复注册
     * - 注册后Reactor开始监控该Handler的事件
     */
    void registerHandler(EventHandler* handler);

    /**
     * @brief 移除Handler
     * @param handler 要移除的事件处理器
     * 
     * 移除流程：
     * 1. 从m_handlers映射表删除
     * 2. 调用selector.removeFd()停止监控
     * 
     * 注意：
     * - 通常在handleClose()之后调用
     * - 移除后Handler不再响应任何事件
     */
    void removeHandler(EventHandler* handler);

    /**
     * @brief 启动事件循环
     * 
     * 事件循环是Reactor的核心：
     * while (m_running) {
     *     numEvents = selector.wait(timeout);
     *     if (numEvents > 0) {
     *         handleEvents();  // 分发到Handler
     *     }
     * }
     * 
     * 阻塞方式：
     * - timeout=-1：永久阻塞，直到有事件发生
     * - timeout=0：立即返回（非阻塞轮询）
     * - timeout>0：最多等待timeout毫秒
     * 
     * 注意：
     * - 构造函数中调用此方法启动服务器
     * - SIGINT/SIGTERM信号触发stop()退出循环
     */
    void loop();

    /**
     * @brief 停止事件循环
     * 
     * 设置m_running=false，下次wait()返回后退出循环
     * 
     * 调用时机：
     * - 收到SIGINT（Ctrl+C）
     * - 收到SIGTERM（kill命令）
     * - 其他需要优雅退出的场景
     */
    void stop();

    /**
     * @brief 是否有运行中的Handler
     * @return true表示有Handler，false表示无
     * 
     * 用于判断是否应该继续运行
     * 当所有连接都断开时，可以选择退出
     */
    bool hasHandlers() const;

private:
    /**
     * @brief 处理所有就绪事件
     * 
     * 遍历selector.getReadyFds()，逐个分发事件：
     * 1. 根据fd找到对应的Handler
     * 2. 根据事件类型调用handleRead/handleWrite
     * 3. 错误或连接关闭时调用handleClose并移除Handler
     * 
     * 处理顺序：
     * - 先处理READ事件（通常先收数据）
     * - 再处理WRITE事件（发送响应）
     * - 最后处理ERROR事件（错误处理）
     */
    void handleEvents();

    Selector m_selector;                      // I/O多路复用封装（epoll）
    std::map<int, EventHandler*> m_handlers; // fd到Handler的映射表
    bool m_running;                          // 事件循环运行标志
};

#endif // REACTOR_H
