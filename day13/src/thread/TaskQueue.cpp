/**
 * @file TaskQueue.cpp
 * @brief 任务队列实现
 */

#include "TaskQueue.h"
#include <iostream>

TaskQueue::TaskQueue(size_t maxSize)
    : m_closed(false)
    , m_maxSize(maxSize)
    , m_taskCount(0)
{
}

TaskQueue::~TaskQueue()
{
    close();
}

bool TaskQueue::push(Task task)
{
    if (m_closed.load()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 检查队列是否已满
        if (m_maxSize > 0 && m_queue.size() >= m_maxSize) {
            return false;
        }
        
        m_queue.push(std::move(task));
        m_taskCount.fetch_add(1);
    }
    
    // 通知一个等待中的线程
    m_cond.notify_one();
    return true;
}

TaskQueue::Task TaskQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // 等待条件：队列非空或已关闭,（wait 期间会自动释放锁，被唤醒后重新加锁）
    m_cond.wait(lock, [this]() {
        return !m_queue.empty() || m_closed.load();
    });
    
    // 如果队列已关闭且为空，返回空任务
    if (m_queue.empty()) {
        return Task();
    }
    
    Task task = std::move(m_queue.front());
    m_queue.pop();
    m_taskCount.fetch_sub(1);
    return task;
}

bool TaskQueue::tryPop(Task& task)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_queue.empty()) {
        return false;
    }
    
    task = std::move(m_queue.front());
    m_queue.pop();
    m_taskCount.fetch_sub(1);
    return true;
}

void TaskQueue::close()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_closed.load()) {
            return;
        }
        m_closed.store(true);
    }
    
    // 通知所有等待中的线程
    m_cond.notify_all();
}

bool TaskQueue::empty() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t TaskQueue::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}