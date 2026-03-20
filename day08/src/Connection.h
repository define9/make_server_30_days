/**
 * @file Connection.h
 * @brief 客户端连接封装 - Day07 Reactor版本
 * 
 * 本文件实现了Reactor模式中的Connection（连接）事件处理器。
 * 
 * Reactor模式中的Connection职责：
 * - 持有客户端连接的fd和地址信息
 * - 管理读写缓冲区（应对非阻塞I/O的半包问题）
 * - 实现EventHandler接口，供Reactor统一调度
 * 
 * 生命周期：
 * 1. Server.handleRead() accept新连接 → 创建Connection对象
 * 2. Reactor注册Connection，开始监控其fd事件
 * 3. 事件触发时Reactor调用handleRead/handleWrite
 * 4. 连接断开时handleClose()清理资源
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "EventHandler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string>

/**
 * @brief 连接状态枚举
 * 
 * 设计理由：
 * - CONNECTED: 正常状态，可读写
 * - CLOSING: 正在关闭（避免重复关闭）
 * - CLOSED: 已关闭（资源已释放）
 */
enum class ConnState {
    CONNECTED,    // 正常连接，可处理I/O
    CLOSING,      // 收到关闭信号，等待处理完剩余数据
    CLOSED        // 已完全关闭，fd已释放
};

/**
 * @brief 连接事件处理器 - 处理单个客户端连接
 * 
 * 继承自EventHandler，是Reactor模式中的"事件处理器"组件。
 * 每个Connection对应一个客户端连接，负责：
 * - 非阻塞读取客户端数据（handleRead）
 * - 非阻塞发送数据给客户端（handleWrite）
 * - 处理连接关闭（handleClose）
 * 
 * 设计要点：
 * - 使用移动语义管理资源，避免深拷贝
 * - 读写缓冲区分离，应对边缘触发的多次通知
 * - fd设置为非阻塞，配合ET模式工作
 */
class Connection : public EventHandler {
public:
    /**
     * @brief 构造函数
     * @param fd 已accept的客户端socket fd
     * @param addr 客户端地址信息
     * 
     * 注意：构造函数中直接将fd设置为非阻塞模式
     * 这是必要的，因为Reactor使用ET(边缘触发)模式
     */
    Connection(int fd, const sockaddr_in& addr);
    
    /**
     * @brief 析构函数 - 确保fd被关闭
     */
    ~Connection();

    // 禁用拷贝构造和拷贝赋值（Connection拥有fd资源）
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 被移动的Connection，原对象的fd置为-1
     * 
     * 移动后，原Connection对象变为无效状态，不能再使用
     */
    Connection(Connection&& other) noexcept;
    
    /**
     * @brief 移动赋值运算符
     * @return 引用自身
     */
    Connection& operator=(Connection&& other) noexcept;

    // ============ EventHandler接口实现 ============
    
    /**
     * @brief 获取连接的文件描述符
     * @return 客户端socket fd
     */
    int fd() const override { return m_fd; }
    
    /**
     * @brief 处理读事件 - 从客户端读取数据
     * @return 成功返回true，客户端断开返回false
     * 
     * 流程：
     * 1. 非阻塞read循环读取所有数据到m_readBuffer
     * 2. 将读取的数据Echo到m_writeBuffer（简单回显服务器逻辑）
     * 3. 读完返回true，连接断开返回false
     */
    bool handleRead() override;
    
    /**
     * @brief 处理写事件 - 发送数据给客户端
     * @return 成功返回true，失败返回false
     * 
     * 流程：
     * 1. 非阻塞write循环发送m_writeBuffer中的所有数据
     * 2. 发送完所有数据后清空缓冲区
     * 3. 若发送缓冲满（EAGAIN），下次继续发送
     */
    bool handleWrite() override;
    
    /**
     * @brief 处理关闭事件 - 清理连接资源
     * 
     * 关闭fd，将状态设为CLOSED
     * Reactor会自动调用此方法并移除该Handler
     */
    void handleClose() override;
    
    /**
     * @brief 返回当前感兴趣的事件类型
     * @return READ或READ|WRITE
     * 
     * 设计决策：
     * - 无数据待发送时，只关心READ事件
     * - 有数据待发送时，同时关心READ和WRITE事件
     * 这样避免不必要的WRITE触发，减少CPU开销
     */
    int interest() const override;
    
    /**
     * @brief 是否有数据等待发送
     * @return true表示有数据，false表示发送缓冲区为空
     */
    bool hasDataToSend() const override { return !m_writeBuffer.empty(); }

    // ============ 辅助方法 ============
    
    /**
     * @brief 获取客户端地址信息
     * @return "IP:Port"格式的字符串
     */
    std::string getClientInfo() const;
    
    /**
     * @brief 连接是否已关闭
     */
    bool isClosed() const { return m_state == ConnState::CLOSED; }
    
    /**
     * @brief 获取当前连接状态
     */
    ConnState state() const { return m_state; }

private:
    int m_fd;                      // 客户端socket文件描述符
    sockaddr_in m_addr;            // 客户端地址信息
    ConnState m_state;            // 当前连接状态
    std::string m_readBuffer;     // 读缓冲区（暂存read()数据）
    std::string m_writeBuffer;    // 写缓冲区（待发送给客户端的数据）
};

#endif // CONNECTION_H
