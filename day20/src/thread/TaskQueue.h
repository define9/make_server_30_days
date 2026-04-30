/**
 * @file TaskQueue.h
 * @brief 任务队列 - 线程安全的任务分发队列
 * 
 * ==================== 任务队列设计 ====================
 * 
 * TaskQueue是线程池的核心组件，负责在生产者（主线程）
 * 和消费者（Worker线程）之间传递任务。
 * 
 * 线程安全机制：
 * 1. std::mutex - 保护队列操作
 * 2. std::condition_variable - 阻塞等待任务到来
 * 3. 原子操作 - 任务计数
 * 
 * ==================== 任务类型 ====================
 * 
 * 使用std::function<void()>表示任务：
 * - 无参数
 * - 无返回值
 * - 可以绑定任意可调用对象（函数、lambda、成员函数）
 */

#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/**
 * @brief 任务队列 - 线程安全的任务分发队列
 * 
 * 模板参数T是任务类型，默认是std::function<void()>。
 * 
 * 特性：
 * - 线程安全：所有public方法都是线程安全的
 * - 阻塞获取：当队列为空时，pop()会阻塞
 * - 有界/无界：可以设置最大任务数
 */
class TaskQueue {
public:
    using Task = std::function<void()>;

    /**
     * @brief 构造函数
     * @param maxSize 最大任务数（0表示无限制）
     */
    explicit TaskQueue(size_t maxSize = 0);

    /**
     * @brief 析构函数
     */
    ~TaskQueue();

    // 禁用拷贝
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    /**
     * @brief 添加任务到队列
     * @param task 任务函数
     * @return true添加成功，false队列已满
     */
    bool push(Task task);

    /**
     * @brief 获取任务（阻塞等待）
     * @return 任务函数
     * 
     * 当队列为空时会阻塞，直到有新任务或队列关闭。
     */
    Task pop();

    /**
     * @brief 尝试获取任务（非阻塞）
     * @param task 输出参数，获取的任务
     * @return true获取成功，false队列为空
     */
    bool tryPop(Task& task);

    /**
     * @brief 关闭队列
     * 
     * 关闭后，pop()会立即返回空任务。
     * 用于通知Worker线程退出。
     */
    void close();

    /**
     * @brief 队列是否为空
     */
    bool empty() const;

    /**
     * @brief 队列中的任务数
     */
    size_t size() const;

    /**
     * @brief 等待中的任务数
     */
    size_t pendingTasks() const { return m_taskCount.load(); }

private:
    std::queue<Task> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
    std::atomic<bool> m_closed;
    size_t m_maxSize;
    // 冗余存一份 count, 因为频繁读取 m_queue 会频繁加锁
    std::atomic<size_t> m_taskCount;
};

#endif // TASK_QUEUE_H