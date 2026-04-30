#ifndef TIMER_WHEEL_H
#define TIMER_WHEEL_H

#include "Timer.h"
#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <cstring>

// 时间轮定时器：O(1) 添加/删除，适合大量定时器
// 原理：环形缓冲区 + 槽，每个槽存放到期时间在该时间范围的定时器
class TimerWheel : public Timer {
public:
    explicit TimerWheel(int64_t tickMs = 100);
    ~TimerWheel() override;

    bool addTimer(int64_t id, int64_t timeout, int64_t interval, TimerCallback callback) override;
    bool removeTimer(int64_t id) override;
    int64_t tick() override;
    int64_t getNextExpire() const override;
    size_t size() const override;

private:
    struct Node {
        int64_t id;
        int64_t expireTime;
        int64_t interval;
        TimerCallback callback;
        bool repeat() const { return interval > 0; }
    };
    
    void cascade(int slot, std::vector<std::pair<int64_t, TimerCallback>>& toRun);
    int64_t currentTime() const;

    // Slot: 槽，每个槽是一个定时器链表
    // 时间轮转动时，到期的定时器被触发或重新定位
    struct Slot {
        std::list<Node> timers;
    };

    int64_t m_tickMs;           // 时间轮tick周期（毫秒）
    int64_t m_currentTime;      // 当前指针位置
    int m_slotCount;            // 槽数量
    int m_slotMask;              // 取模掩码 (slotCount-1)
    Slot* m_slots;              // 槽数组
    // 定时器ID -> (槽索引, 链表迭代器) 的映射，用于O(1)删除
    std::map<int64_t, std::pair<int, std::list<Node>::iterator>> m_timerMap;
    mutable std::mutex m_mutex;
    int64_t m_timerIdCounter;
};

#endif // TIMER_WHEEL_H