#ifndef TIMER_HEAP_H
#define TIMER_HEAP_H

#include "Timer.h"
#include <vector>
#include <map>
#include <mutex>
#include <sys/time.h>

// 最小堆定时器：O(log n) 添加/删除
// 堆顶是最早到期的定时器，适合定时器数量较少的场景
class TimerHeap : public Timer {
public:
    TimerHeap();
    ~TimerHeap() override;

    bool addTimer(int64_t id, int64_t timeout, int64_t interval, TimerCallback callback) override;
    bool removeTimer(int64_t id) override;
    int64_t tick() override;
    int64_t getNextExpire() const override;
    size_t size() const override;

private:
    int64_t currentTime() const;
    void percolateDown(int hole);
    void percolateUp(int hole);

    struct Node {
        int64_t id;
        int64_t expireTime;
        int64_t interval;
        TimerCallback callback;
        bool repeat() const { return interval > 0; }
    };

    std::vector<Node> m_heap;           // 最小堆数组
    std::map<int64_t, size_t> m_indexMap;  // ID -> 堆索引映射，用于O(1)删除
    mutable std::mutex m_mutex;
    int64_t m_timerIdCounter;
};

#endif // TIMER_HEAP_H