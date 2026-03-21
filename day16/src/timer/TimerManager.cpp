#include "TimerManager.h"
#include <iostream>

TimerManager::TimerManager(TimerType type, int64_t tickMs)
    : m_type(type)
    , m_idCounter(0)
{
    if (type == TimerType::HEAP) {
        m_timer = std::make_unique<TimerHeap>();
    } else {
        m_timer = std::make_unique<TimerWheel>(tickMs);
    }
}

TimerManager::~TimerManager() {}

int64_t TimerManager::addTimer(int64_t timeout, int64_t interval, TimerCallback callback) {
    int64_t id = ++m_idCounter;
    if (m_timer->addTimer(id, timeout, interval, callback)) {
        return id;
    }
    return -1;
}

bool TimerManager::removeTimer(int64_t id) {
    return m_timer->removeTimer(id);
}

int64_t TimerManager::tick() {
    return m_timer->tick();
}

int64_t TimerManager::getNextExpire() const {
    return m_timer->getNextExpire();
}

size_t TimerManager::size() const {
    return m_timer->size();
}