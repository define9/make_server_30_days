/**
 * @file ThreadPool.cpp
 * @brief 线程池实现
 */

#include "ThreadPool.h"
#include "log/Logger.h"
#include <chrono>

ThreadPool::ThreadPool(size_t numThreads, size_t queueSize)
    : m_taskQueue(queueSize)
    , m_running(false)
    , m_activeTasks(0)
    , m_numThreads(numThreads)
{
}

ThreadPool::~ThreadPool()
{
    stop();
}

void ThreadPool::start()
{
    if (m_running.load()) {
        return;
    }

    m_running.store(true);

    // 创建Worker线程
    for (size_t i = 0; i < m_numThreads; ++i) {
        m_threads.emplace_back([this, i]() {
            workerThread(i);
        });
    }

    LOG_INFO("Started %zu worker threads", m_numThreads);
}

void ThreadPool::workerThread(size_t workerId)
{
    LOG_DEBUG("Worker %zu started", workerId);

    while (m_running.load()) {
        // 从队列获取任务（阻塞）
        TaskQueue::Task task = m_taskQueue.pop();

        // 检查是否应该退出
        if (!m_running.load() && !task) {
            break;
        }

        // 执行任务
        if (task) {
            try {
                m_activeTasks.fetch_add(1);
                
                task();

                m_activeTasks.fetch_sub(1);
            } catch (const std::exception& e) {
                LOG_ERROR("Worker %zu exception: %s", workerId, e.what());
            } catch (...) {
                LOG_ERROR("Worker %zu unknown exception", workerId);
            }
        }
    }

    LOG_DEBUG("Worker %zu stopped", workerId);
}

void ThreadPool::stop()
{
    if (!m_running.load()) {
        return;
    }

    m_running.store(false);

    // 关闭任务队列（唤醒所有等待的线程）
    m_taskQueue.close();

    // 等待所有Worker线程结束
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    m_threads.clear();
    LOG_INFO("All workers stopped");
}

void ThreadPool::waitForIdle()
{    
    // 等待直到队列为空
    while (m_taskQueue.pendingTasks() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_DEBUG("tasks queue completed");

    while (m_activeTasks.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_DEBUG("active tasks queue completed");
}