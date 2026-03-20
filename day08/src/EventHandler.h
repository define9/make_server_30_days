/**
 * @file EventHandler.h
 * @brief 事件处理器抽象接口 - Day07 Reactor模式基础
 * 
 * 本文件定义了Reactor模式中的核心抽象：EventHandler（事件处理器接口）
 * 
 * ==================== Reactor模式简介 ====================
 * 
 * Reactor模式是一种高效的事件驱动架构模式，广泛应用于高性能服务器开发。
 * 核心思想：将I/O事件的侦测（多路复用）和处理解耦，由Reactor统一协调。
 * 
 * 模式组件：
 * 1. EventHandler（事件处理器）：定义处理事件的抽象接口
 *    - 每个需要处理I/O的对象实现此接口
 *    - 提供handleRead/handleWrite/handleClose等方法
 * 
 * 2. Reactor（反应器）：事件分发中心
 *    - 持有Selector进行I/O多路复用
 *    - 管理所有Handler的注册
 *    - 事件循环：等待事件 → 分发到对应Handler
 * 
 * 3. EventSource（事件源）：产生事件的地方
 *    - 如Server（监听端口，accept新连接）
 *    - 如Connection（已建立的客户端连接）
 * 
 * ==================== Day07核心变化 ====================
 * 
 * - 引入EventHandler抽象接口
 * - 所有事件处理器(Connection, Server)实现此接口
 * - Reactor通过此接口统一调度事件
 * - 实现了真正的"控制反转"：框架调用用户代码，而非反过来
 * 
 * ==================== 设计优势 ====================
 * 
 * 1. 解耦：事件检测和事件处理分离
 *    - Reactor负责"什么时候做"
 *    - Handler负责"怎么做"
 * 
 * 2. 可扩展：新增事件类型只需实现EventHandler
 *    - 无需修改Reactor代码（开放-封闭原则）
 * 
 * 3. 可测试：每个Handler可独立测试
 *    - Mock Reactor即可单元测试Handler
 */

#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "Selector.h"  // for EventType

/**
 * @brief 事件处理器接口 - Reactor模式的核心抽象
 * 
 * 所有需要处理I/O事件的对象都必须实现此接口。
 * Reactor通过此接口统一调度所有事件。
 * 
 * 接口设计原则：
 * - 虚析构函数：确保子类正确销毁
 * - 纯虚方法：强制子类实现核心功能
 * - 默认实现：hasDataToSend默认返回false
 * 
 * 使用示例：
 * @code
 * class MyConnection : public EventHandler {
 * public:
 *     int fd() const override { return m_fd; }
 *     bool handleRead() override { 处理读事件 }
 *     bool handleWrite() override { 处理写事件 }
 *     void handleClose() override { 清理资源 }
 *     int interest() const override { return static_cast<int>(EventType::READ); }
 *     bool hasDataToSend() const override { return !m_sendQueue.empty(); }
 * private:
 *     int m_fd;
 * };
 * @endcode
 */
class EventHandler {
public:
    /**
     * @brief 析构函数
     * 
     * 必须为virtual，确保通过基类指针删除子类对象时正确调用析构函数
     */
    virtual ~EventHandler() = default;

    /**
     * @brief 获取Handler关联的文件描述符
     * @return 文件描述符（socket fd）
     * 
     * 设计理由：
     * - Reactor需要用fd来标识和查找Handler
     * - 每个Handler必须关联一个fd
     * - fd是Reactor管理Handler的唯一键
     */
    virtual int fd() const = 0;

    /**
     * @brief 处理读事件
     * @return 成功返回true，失败返回false（连接断开等）
     * 
     * 调用时机：
     * - fd变为可读（对端发送数据、监听fd可accept）
     * - Reactor从wait()返回后，调用对应的handleRead()
     * 
     * 子类实现要点：
     * - Server: 调用accept()接受新连接
     * - Connection: 调用read()读取数据
     * - 返回false表示需要关闭该Handler
     */
    virtual bool handleRead() = 0;

    /**
     * @brief 处理写事件
     * @return 成功返回true，失败返回false
     * 
     * 调用时机：
     * - fd变为可写（发送缓冲区有空间）
     * - 通常只在有数据要发送时注册此事件
     * 
     * 子类实现要点：
     * - Connection: 调用write()发送数据
     * - 如果数据发送完毕，返回true并通知Reactor停止监控WRITE
     */
    virtual bool handleWrite() = 0;

    /**
     * @brief 处理关闭事件
     * 
     * 调用时机：
     * - 检测到错误（EPOLLERR）
     * - 连接正常断开
     * - Reactor决定移除该Handler时
     * 
     * 子类实现要点：
     * - 关闭fd
     * - 释放资源
     * - 更新状态
     */
    virtual void handleClose() = 0;

    /**
     * @brief 获取Handler感兴趣的事件类型
     * @return 需要Reactor监控的事件类型（位掩码）
     * 
     * 设计决策：让Handler自己声明关注什么事件
     * 
     * 典型返回值：
     * - Server: 只返回READ（只关心accept）
     * - Connection无数据发送: 只返回READ（等待数据）
     * - Connection有数据发送: 返回READ|WRITE（既要收也要发）
     * 
     * 为什么需要动态返回？
     * - 避免不必要的WRITE事件通知（空转CPU）
     * - 根据Handler状态动态调整关注的事件
     * 
     * @note 返回值是EventType枚举值的组合，使用|运算符
     */
    virtual int interest() const = 0;

    /**
     * @brief 是否有数据等待发送
     * @return true表示有数据，false表示没有
     * 
     * 设计理由：
     * - Reactor需要知道Handler是否有数据要发送
     * - 有数据时才注册WRITE事件，避免空转
     * - 发送完毕后返回false，Reactor停止监控WRITE
     * 
     * 默认实现：返回false（无数据发送需求）
     */
    virtual bool hasDataToSend() const = 0;
};

#endif // EVENT_HANDLER_H
