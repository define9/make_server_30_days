#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include "Timer.h"
#include "TimerHeap.h"
#include "TimerWheel.h"
#include <memory>
#include <atomic>

enum class TimerType {
    HEAP,
    WHEEL
};

class TimerManager {
public:
    explicit TimerManager(TimerType type = TimerType::HEAP, int64_t tickMs = 100);
    ~TimerManager();

    int64_t addTimer(int64_t timeout, int64_t interval, TimerCallback callback);
    bool removeTimer(int64_t id);
    int64_t tick();
    int64_t getNextExpire() const;
    size_t size() const;

private:
    std::unique_ptr<Timer> m_timer;
    TimerType m_type;
    std::atomic<int64_t> m_idCounter;
};

#endif // TIMER_MANAGER_H