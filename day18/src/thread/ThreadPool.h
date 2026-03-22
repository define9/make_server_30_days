/**
 * @file ThreadPool.h
 * @brief 线程池 - 生产者-消费者模式实现
 * 
 * ==================== 线程池架构 ====================
 * 
 * 线程池是一种常见的并发设计模式，用于管理多个Worker线程：
 * 
 *     ┌─────────────────────────────────────────────┐
 *     │              ThreadPool                      │
 *     │  ┌───────────────────────────────────────┐  │
 *     │  │         TaskQueue (任务队列)            │  │
 *     │  │   [task1] [task2] [task3] ...         │  │
 *     │  └───────────────────────────────────────┘  │
 *     │        ▲                    │              │
 *     │        │                    │              │
 *     │   生产者                消费者              │
 *     │   (主线程)             (Worker线程)         │
 *     └─────────────────────────────────────────────┘
 *                      │
 *     ┌─────────────────┼─────────────────┐
 *     ▼                 ▼                 ▼
 * ┌─────────┐     ┌─────────┐     ┌─────────┐
 * │Worker 0 │     │Worker 1 │     │Worker 2 │
 * └─────────┘     └─────────┘     └─────────┘
 * 
 * ==================== 核心组件 ====================
 * 
 * 1. TaskQueue - 线程安全的任务队列
 * 2. Worker线程 - 从队列取任务并执行
 * 3. ThreadPool - 管理Worker线程和任务调度
 * 
 * ==================== 特性 ====================
 * 
 * - 懒加载：Worker线程在需要时才创建
 * - 优雅退出：close()后等待所有任务完成
 * - 可配置：线程数、队列大小可调
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "TaskQueue.h"
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

/**
 * @brief 线程池类
 * 
 * 管理一组Worker线程，从任务队列中获取任务并执行。
 * 
 * 使用方式：
 * 1. 构造函数指定线程数和队列大小
 * 2. start()启动所有Worker线程
 * 3. submit()提交任务
 * 4. stop()停止线程池
 */
class ThreadPool {
public:
    /**
     * @brief 构造函数
     * @param numThreads 工作线程数（默认4）
     * @param queueSize 任务队列大小（0表示无限制）
     */
    explicit ThreadPool(size_t numThreads = 4, size_t queueSize = 0);

    /**
     * @brief 析构函数 - 自动停止线程池
     */
    ~ThreadPool();

    // 禁用拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief 启动线程池
     * 
     * 创建Worker线程并开始处理任务。
     */
    void start();

    /**
     * @brief 提交任务
     * @param task 任务函数
     * @return true提交成功，false队列已满
     */
    template<typename F>
    bool submit(F task);

    /**
     * @brief 提交任务（带优先级）
     * @param task 任务函数
     * @param priority 优先级（暂未实现）
     */
    template<typename F>
    bool submitPriority(F task, int priority);

    /**
     * @brief 停止线程池（优雅停止）
     * 
     * 停止接受新任务，等待队列中的任务完成后再退出。
     */
    void stop();

    /**
     * @brief 等待所有任务完成
     * 
     * 阻塞直到队列中的所有任务都被执行。
     */
    void waitForIdle();

    /**
     * @brief 线程池是否正在运行
     */
    bool isRunning() const { return m_running.load(); }

    /**
     * @brief 获取Worker线程数
     */
    size_t numThreads() const { return m_threads.size(); }

    /**
     * @brief 获取待处理任务数
     */
    size_t pendingTasks() const { return m_taskQueue.pendingTasks(); }

private:
    void workerThread(size_t workerId);

    TaskQueue m_taskQueue;
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_running;
    // 当前激活 正在执行的 task
    std::atomic<size_t> m_activeTasks;
    size_t m_numThreads;
};

template<typename F>
bool ThreadPool::submit(F task)
{
    if (!m_running.load()) {
        return false;
    }
    return m_taskQueue.push(std::function<void()>(task));
}

template<typename F>
bool ThreadPool::submitPriority(F task, int priority)
{
    // 优先级队列暂未实现，目前简单调用submit
    (void)priority;
    return submit(std::forward<F>(task));
}

#endif // THREAD_POOL_H