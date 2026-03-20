/**
 * @file EventHandler.h
 * @brief 事件处理器抽象接口 - Day09 多线程Reactor版本
 */

#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "Selector.h"

/**
 * @brief 事件处理器接口
 */
class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual int fd() const = 0;
    virtual bool handleRead() = 0;
    virtual bool handleWrite() = 0;
    virtual void handleClose() = 0;
    virtual int interest() const = 0;
    virtual bool hasDataToSend() const = 0;
};

#endif // EVENT_HANDLER_H