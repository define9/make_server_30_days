/**
 * @file Reactor.cpp
 * @brief Reactor实现
 * 
 * 本文件实现Reactor模式的Reactor组件。
 */

#include "Reactor.h"
#include "EventHandler.h"
#include <iostream>
#include <vector>
#include <errno.h>

/**
 * @brief 构造函数
 * 
 * 初始化运行标志为false
 * Selector在构造时自动创建epoll实例
 */
Reactor::Reactor()
    : m_running(false)
{
}

/**
 * @brief 析构函数
 * 
 * 确保事件循环被停止，然后清理Selector
 */
Reactor::~Reactor()
{
    stop();
}

/**
 * @brief 注册Handler
 * 
 * 实现步骤：
 * 1. 参数检查（防止空指针）
 * 2. 将fd和handler加入映射表
 * 3. 告诉Selector开始监控该fd的事件
 * 
 * @param handler 要注册的Handler（不能为nullptr）
 */
void Reactor::registerHandler(EventHandler* handler)
{
    if (!handler) return;

    int fd = handler->fd();
    // fd作为键，handler作为值
    m_handlers[fd] = handler;
    // 告诉Selector监控该fd
    m_selector.addFd(fd, handler->interest());
}

/**
 * @brief 移除Handler
 * 
 * 实现步骤：
 * 1. 参数检查
 * 2. 从映射表删除
 * 3. 告诉Selector停止监控该fd
 * 
 * 注意：只从Reactor移除，不负责关闭Handler
 * 关闭操作由handleClose()负责
 * 
 * @param handler 要移除的Handler
 */
void Reactor::removeHandler(EventHandler* handler)
{
    if (!handler) return;

    int fd = handler->fd();
    m_handlers.erase(fd);
    m_selector.removeFd(fd);

    delete handler;
}

/**
 * @brief 事件循环 - Reactor的核心
 * 
 * 事件循环流程：
 * 1. 设置运行标志为true
 * 2. 进入循环：while (m_running)
 * 3. 调用selector.wait()阻塞等待事件
 * 4. 有事件发生：调用handleEvents()分发
 * 5. 无事件发生：继续等待
 * 6. stop()被调用：退出循环
 * 
 * 关于wait()超时：
 * - 这里使用1000ms超时，避免长时间阻塞
 * - 超时后继续下一次循环（可处理定时任务、日志刷新等）
 * - EINTR表示被信号中断，继续等待
 */
void Reactor::loop()
{
    m_running = true;
    std::cout << "[Reactor] Started" << std::endl;

    while (m_running) {
        // 等待1000ms，兼容定时任务处理
        int numEvents = m_selector.wait(1000);

        // 错误处理
        if (numEvents < 0) {
            if (errno == EINTR) {
                // EINTR：被信号中断，不是错误，继续等待
                continue;
            }
            std::cerr << "[Reactor] epoll_wait() error" << std::endl;
            break;
        }

        // 无事件发生（超时），继续等待
        if (numEvents == 0) {
            continue;
        }

        // 处理所有就绪事件
        handleEvents();
    }

    std::cout << "[Reactor] Stopped" << std::endl;
}

/**
 * @brief 停止事件循环
 * 
 * 设置m_running=false。
 * 注意：不会立即退出，而是等待当前wait()返回后才退出。
 */
void Reactor::stop()
{
    m_running = false;
}

/**
 * @brief 检查是否有运行的Handler
 */
bool Reactor::hasHandlers() const
{
    return !m_handlers.empty();
}

/**
 * @brief 处理所有就绪事件
 * 
 * 这是Reactor的"事件分发"逻辑。
 * 
 * 分发策略：
 * 1. 遍历所有就绪的fd
 * 2. 根据fd找到对应的Handler
 * 3. 根据事件类型调用相应方法
 * 4. 处理完后检查是否需要更新关注的事件
 * 
 * 处理优先级：
 * - READ > WRITE > ERROR
 * - 先处理读（通常需要先收数据才能发）
 * - 最后统一处理需要关闭的Handler
 * 
 * 重要：不在遍历中直接removeHandler，而是收集到toRemove列表
 * 原因：遍历时修改map可能导致迭代器失效
 */
void Reactor::handleEvents()
{
    // 获取就绪的fd列表
    const auto& readyFds = m_selector.getReadyFds();
    
    // 收集需要移除的Handler（在遍历结束后统一处理）
    std::vector<EventHandler*> toRemove;

    // 遍历所有就绪事件
    for (const auto& pair : readyFds) {
        int fd = pair.first;
        EventType event = pair.second;

        // 查找对应的Handler
        auto it = m_handlers.find(fd);
        if (it == m_handlers.end()) {
            // 该fd没有对应的Handler（可能被移除了）
            continue;
        }
        EventHandler* handler = it->second;

        // ===== 处理READ事件 =====
        if (static_cast<int>(event) & static_cast<int>(EventType::READ)) {
            if (!handler->handleRead()) {
                // handleRead返回false表示需要关闭
                toRemove.push_back(handler);
                continue;
            }
            // 读取成功后，检查是否现在需要发送数据
            // 如果之前没数据，现在有了，需要注册WRITE事件
            if (handler->hasDataToSend()) {
                m_selector.modifyFd(fd, handler->interest());
            }
        }

        // ===== 处理WRITE事件 =====
        if (static_cast<int>(event) & static_cast<int>(EventType::WRITE)) {
            if (!handler->handleWrite()) {
                toRemove.push_back(handler);
                continue;
            }
            // 发送完毕后，不再需要WRITE事件，只关心READ
            if (!handler->hasDataToSend()) {
                m_selector.modifyFd(fd, EventType::READ);
            }
        }

        // ===== 处理ERROR事件 =====
        if (static_cast<int>(event) & static_cast<int>(EventType::ERROR)) {
            std::cerr << "[Reactor] Error on fd " << fd << std::endl;
            toRemove.push_back(handler);
        }
    }

    // ===== 统一处理需要关闭的Handler =====
    // 这样做的好处是避免在遍历中修改map导致迭代器失效
    for (EventHandler* handler : toRemove) {
        handler->handleClose();   // 通知Handler关闭（清理资源）
        removeHandler(handler);   // 从Reactor移除
    }
}
