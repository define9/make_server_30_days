/**
 * @file ThreadPool.cpp
 * @brief 线程池实现
 */

#include "ThreadPool.h"
#include <iostream>
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

    std::cout << "[ThreadPool] Started " << m_numThreads << " worker threads" << std::endl;
}

void ThreadPool::workerThread(size_t workerId)
{
    std::cout << "[ThreadPool] Worker " << workerId << " started" << std::endl;

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
                std::cerr << "[ThreadPool] Worker " << workerId 
                          << " exception: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ThreadPool] Worker " << workerId 
                          << " unknown exception" << std::endl;
            }
        }
    }

    std::cout << "[ThreadPool] Worker " << workerId << " stopped" << std::endl;
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
    std::cout << "[ThreadPool] All workers stopped" << std::endl;
}

void ThreadPool::waitForIdle()
{    
    // 等待直到队列为空
    while (m_taskQueue.pendingTasks() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "[ThreadPool] tasks queue completed" << std::endl;

    while (m_activeTasks.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "[ThreadPool] active tasks queue completed" << std::endl;
}