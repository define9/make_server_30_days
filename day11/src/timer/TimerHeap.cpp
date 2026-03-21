#include "TimerHeap.h"
#include <iostream>
#include <limits>

TimerHeap::TimerHeap() : m_timerIdCounter(0) {
    m_heap.reserve(64);
}

TimerHeap::~TimerHeap() {}

int64_t TimerHeap::currentTime() const {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 最小堆下沉操作：用于删除节点后维护堆性质
// hole: 要下沉的起始位置
void TimerHeap::percolateDown(int hole) {
    int child;
    Node tmp = m_heap[hole];
    int n = static_cast<int>(m_heap.size());
    
    for (; hole * 2 + 1 < n; hole = child) {
        child = hole * 2 + 1;
        // 选择较小的子节点
        if (child + 1 < n && m_heap[child + 1].expireTime < m_heap[child].expireTime) {
            child++;
        }
        // 如果子节点更小，则上移
        if (m_heap[child].expireTime < tmp.expireTime) {
            m_heap[hole] = m_heap[child];
            m_indexMap[m_heap[hole].id] = hole;
        } else {
            break;
        }
    }
    m_heap[hole] = tmp;
    m_indexMap[tmp.id] = hole;
}

// 最小堆上浮操作：用于插入节点后维护堆性质
// hole: 要上浮的起始位置
void TimerHeap::percolateUp(int hole) {
    Node tmp = m_heap[hole];
    int parent;
    
    for (; hole > 0; hole = parent) {
        parent = (hole - 1) / 2;
        // 如果父节点更大，则下移
        if (m_heap[parent].expireTime > tmp.expireTime) {
            m_heap[hole] = m_heap[parent];
            m_indexMap[m_heap[hole].id] = hole;
        } else {
            break;
        }
    }
    m_heap[hole] = tmp;
    m_indexMap[tmp.id] = hole;
}

bool TimerHeap::addTimer(int64_t id, int64_t timeout, int64_t interval, TimerCallback callback) {
    if (timeout <= 0) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 防止重复ID
    if (m_indexMap.find(id) != m_indexMap.end()) {
        return false;
    }
    
    int64_t expireTime = currentTime() + timeout;
    
    Node node;
    node.id = id;
    node.expireTime = expireTime;
    node.interval = interval;
    node.callback = std::move(callback);
    
    // 添加到堆末尾，然后上浮
    m_heap.push_back(node);
    size_t hole = m_heap.size() - 1;
    percolateUp(hole);
    
    return true;
}

bool TimerHeap::removeTimer(int64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_indexMap.find(id);
    if (it == m_indexMap.end()) return false;
    
    size_t hole = it->second;
    size_t last = m_heap.size() - 1;
    
    // 用最后一个节点替换要删除的节点，然后下浮或上浮
    if (hole != last) {
        m_heap[hole] = m_heap[last];
        m_indexMap[m_heap[hole].id] = hole;
        m_heap.pop_back();
        
        if (hole > 0 && m_heap[hole].expireTime < m_heap[(hole - 1) / 2].expireTime) {
            percolateUp(hole);
        } else {
            percolateDown(hole);
        }
    } else {
        m_heap.pop_back();
    }
    
    m_indexMap.erase(it);
    return true;
}

// tick: 检查并触发到期的定时器
// 返回值: 下一个定时器到期还需要的毫秒数，-1表示没有定时器
int64_t TimerHeap::tick() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_heap.empty()) {
        return -1;
    }
    
    int64_t now = currentTime();
    int64_t minExpire = m_heap[0].expireTime;
    
    // 还没到期，返回等待时间
    if (now < minExpire) {
        return minExpire - now;
    }
    
    // 收集所有到期的回调，在锁外执行以避免死锁
    std::vector<std::pair<int64_t, TimerCallback>> toRun;
    
    while (!m_heap.empty() && m_heap[0].expireTime <= now) {
        Node node = m_heap[0];
        
        size_t last = m_heap.size() - 1;
        m_heap[0] = m_heap[last];
        m_indexMap[m_heap[0].id] = 0;
        m_heap.pop_back();
        
        if (!m_heap.empty()) {
            percolateDown(0);
        }
        
        m_indexMap.erase(node.id);
        
        // 周期性定时器重新入堆
        if (node.callback && node.repeat()) {
            TimerCallback cb = node.callback;
            int64_t id = node.id;
            toRun.push_back({id, std::move(cb)});
            node.expireTime = now + node.interval;
            m_heap.push_back(node);
            percolateUp(m_heap.size() - 1);
        } else if (node.callback) {
            toRun.push_back({node.id, std::move(node.callback)});
        }
    }
    
    lock.unlock();
    
    // 触发回调
    for (auto& pair : toRun) {
        if (pair.second) pair.second(pair.first);
    }
    
    lock.lock();
    
    if (m_heap.empty()) {
        return -1;
    }
    
    return m_heap[0].expireTime - now;
}

int64_t TimerHeap::getNextExpire() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_heap.empty()) return -1;
    
    int64_t diff = m_heap[0].expireTime - currentTime();
    return diff > 0 ? diff : 0;
}

size_t TimerHeap::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_heap.size();
}