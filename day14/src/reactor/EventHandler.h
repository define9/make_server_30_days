/**
 * @file EventHandler.h
 * @brief 事件处理器接口 - Reactor模式核心抽象
 */

#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "net/Selector.h"

// 事件处理器接口 - Reactor模式核心抽象
//
// 所有需要被Reactor管理的事件处理器都必须实现此接口。
// Reactor通过fd()获取文件描述符，通过interest()获取关注的事件类型，
// 然后调用handleRead/handleWrite/handleClose处理对应的IO事件。
class EventHandler {
public:
    virtual ~EventHandler() = default;

    // 获取关联的文件描述符
    virtual int fd() const = 0;

    // 处理读事件（对应EPOLLIN）
    // 返回true表示继续关注该事件，返回false表示关闭连接
    virtual bool handleRead() = 0;

    // 处理写事件（对应EPOLLOUT）
    // 返回true表示继续关注该事件，返回false表示关闭连接
    virtual bool handleWrite() = 0;

    // 关闭连接，释放资源
    virtual void handleClose() = 0;

    // 返回关注的事件类型（EPOLLIN/EPOLLOUT组合）
    virtual int interest() const = 0;

    // 是否有数据待发送（用于决定是否关注写事件）
    virtual bool hasDataToSend() const = 0;
};

#endif // EVENT_HANDLER_H