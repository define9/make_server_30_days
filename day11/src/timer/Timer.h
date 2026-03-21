#ifndef TIMER_H
#define TIMER_H

#include <cstdint>
#include <cstddef>
#include <functional>

struct TimerNode {
    int64_t id;
    int64_t expireTime;
    int64_t interval;
    
    bool repeat() const { return interval > 0; }
    TimerNode(int64_t id_ = 0, int64_t expire_ = 0, int64_t interval_ = 0)
        : id(id_), expireTime(expire_), interval(interval_) {}
};

using TimerCallback = std::function<void(int64_t timerId)>;

class Timer {
public:
    virtual ~Timer() = default;
    virtual bool addTimer(int64_t id, int64_t timeout, int64_t interval, TimerCallback callback) = 0;
    virtual bool removeTimer(int64_t id) = 0;
    virtual int64_t tick() = 0;
    virtual int64_t getNextExpire() const = 0;
    virtual size_t size() const = 0;
};

#endif // TIMER_H