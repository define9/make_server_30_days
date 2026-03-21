#include "TimerWheel.h"
#include <iostream>
#include <vector>
#include <sys/time.h>

// 时间轮构造：tickMs=100ms, slotCount=60, 组成6秒的环形缓冲区
TimerWheel::TimerWheel(int64_t tickMs)
    : m_tickMs(tickMs)
    , m_currentTime(0)
    , m_slotCount(60)
    , m_slotMask(59)  // 59 = 60-1，用于取模优化
    , m_timerIdCounter(0)
{
    m_slots = new Slot[m_slotCount];
}

TimerWheel::~TimerWheel() {
    delete[] m_slots;
}

int64_t TimerWheel::currentTime() const {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 添加定时器：根据到期时间计算槽位置
bool TimerWheel::addTimer(int64_t id, int64_t timeout, int64_t interval, TimerCallback callback) {
    if (timeout <= 0 || !callback) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int64_t expireTime = currentTime() + timeout;
    // (expireTime / m_tickMs) & m_slotMask 相当于取模60
    int slot = static_cast<int>((expireTime / m_tickMs) & m_slotMask);
    
    Node node;
    node.id = id;
    node.expireTime = expireTime;
    node.interval = interval;
    node.callback = std::move(callback);
    
    m_slots[slot].timers.push_back(node);
    
    auto it = m_slots[slot].timers.end();
    --it;
    m_timerMap[id] = {slot, it};
    
    return true;
}

bool TimerWheel::removeTimer(int64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_timerMap.find(id);
    if (it == m_timerMap.end()) return false;
    
    int slot = it->second.first;
    auto& listIt = it->second.second;
    
    m_slots[slot].timers.erase(listIt);
    m_timerMap.erase(it);
    
    return true;
}

// cascade: 时间轮推进时处理某槽的定时器
// 将到期的定时器移到更晚的槽或触发回调
void TimerWheel::cascade(int slot, std::vector<std::pair<int64_t, TimerCallback>>& toRun) {
    Slot& s = m_slots[slot];
    for (auto it = s.timers.begin(); it != s.timers.end();) {
        Node& node = *it;
        
        m_timerMap.erase(node.id);
        s.timers.erase(it++);
        
        if (node.callback) {
            toRun.push_back({node.id, std::move(node.callback)});
        }
        
        // 周期性定时器重新计算槽位置
        if (node.repeat()) {
            node.expireTime = currentTime() + node.interval;
            int nextSlot = static_cast<int>((node.expireTime / m_tickMs) & m_slotMask);
            m_slots[nextSlot].timers.push_back(node);
            auto newIt = m_slots[nextSlot].timers.end();
            --newIt;
            m_timerMap[node.id] = {nextSlot, newIt};
        }
    }
}

// tick: 推进时间轮，返回下一个tick的等待时间
int64_t TimerWheel::tick() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    int64_t now = currentTime();
    int64_t elapsed = now - m_currentTime;
    
    if (elapsed < m_tickMs) {
        return m_tickMs - elapsed;
    }
    
    std::vector<std::pair<int64_t, TimerCallback>> toRun;
    int cascadeCount = 0;
    
    // 推进时间轮，每100ms转动一格
    while (elapsed >= m_tickMs) {
        int slot = static_cast<int>(m_currentTime / m_tickMs) & m_slotMask;
        
        if (!m_slots[slot].timers.empty()) {
            cascade(slot, toRun);
        }
        
        m_currentTime += m_tickMs;
        elapsed -= m_tickMs;
        cascadeCount++;
        
        // 防止时间流逝太久导致过度cascade
        if (cascadeCount > m_slotCount) break;
    }
    
    if (elapsed >= m_tickMs) {
        m_currentTime = now;
    }
    
    lock.unlock();
    
    for (auto& pair : toRun) {
        if (pair.second) pair.second(pair.first);
    }
    
    return m_tickMs;
}

int64_t TimerWheel::getNextExpire() const {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (m_timerMap.empty()) return -1;
    
    int64_t earliest = INT64_MAX;
    for (const auto& pair : m_timerMap) {
        const Node& node = *pair.second.second;
        if (node.expireTime < earliest) {
            earliest = node.expireTime;
        }
    }
    
    int64_t diff = earliest - currentTime();
    return diff > 0 ? diff : 0;
}

size_t TimerWheel::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_timerMap.size();
}