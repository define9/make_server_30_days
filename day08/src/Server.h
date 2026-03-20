/**
 * @file Server.h
 * @brief TCP服务器 - Day07 Reactor版本
 * 
 * ==================== Server在Reactor模式中的角色 ====================
 * 
 * Server是Reactor模式中的"事件源"之一。
 * 它的职责相对简单：
 * - 监听端口，接受新连接
 * - 创建Connection对象处理已建立的连接
 * 
 * ==================== 为什么Server也是EventHandler？ ====================
 * 
 * 设计决策：让Server实现EventHandler接口
 * 
 * 理由：
 * 1. 统一管理：所有fd事件都由Reactor分发，无需特殊处理Server
 * 2. 一致性：Reactor只需处理一种类型（EventHandler*）
 * 3. 简单性：Reactor.handleEvents()的逻辑统一
 * 
 * Server的EventHandler行为：
 * - fd()返回监听socket
 * - handleRead()调用accept()接受新连接
 * - handleWrite()不需要（监听socket不写数据）
 * - interest()只返回READ（只关心accept事件）
 * 
 * ==================== Day07核心变化 ====================
 * 
 * - Server不再自己管理事件循环
 * - Server在构造函数中将自己注册到Reactor
 * - Reactor负责调用Server.handleRead()接受新连接
 */

#ifndef SERVER_H
#define SERVER_H

#include "EventHandler.h"
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>

class Reactor;

/**
 * @brief TCP服务器 - Reactor模式的事件源
 * 
 * Server继承自EventHandler，是Reactor模式中的"事件源"。
 * 
 * 职责：
 * 1. 创建监听socket，绑定端口
 * 2. listen()开始监听
 * 3. 将自己注册到Reactor（监听socket的READ事件）
 * 4. 当accept事件触发时，调用accept()接受新连接
 * 5. 创建Connection对象，交给Reactor管理
 * 
 * 特殊设计：
 * - Server只关心READ事件（accept）
 * - 不关心WRITE事件（不会主动发送数据）
 * - hasDataToSend()永远返回false
 * 
 * 生命周期：
 * 1. main()创建Reactor和Server
 * 2. Server构造函数完成初始化并注册到Reactor
 * 3. Reactor.loop()启动事件循环
 * 4. 收到SIGINT/SIGTERM时优雅退出
 */
class Server : public EventHandler {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param reactor 所属的Reactor指针
     * 
     * 构造流程：
     * 1. 初始化成员变量
     * 2. setupServerSocket()创建并配置监听socket
     * 3. 将自己注册到Reactor
     */
    Server(uint16_t port, Reactor* reactor);
    
    /**
     * @brief 析构函数
     */
    ~Server();

    // 禁用拷贝：Server拥有监听socket资源
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // ============ EventHandler接口实现 ============

    /**
     * @brief 获取监听socket的fd
     */
    int fd() const override { return m_serverSocket; }
    
    /**
     * @brief 处理读事件 - accept新连接
     * 
     * 当监听socket可读时，表示有客户端连接请求（TCP backlog中有pending连接）。
     * handleRead()循环调用accept()接受所有pending连接。
     * 
     * @return 总是返回true（Server本身不会出错或关闭）
     */
    bool handleRead() override;
    
    /**
     * @brief 处理写事件 - 不需要
     * @return 总是返回true
     */
    bool handleWrite() override { return true; }
    
    /**
     * @brief 处理关闭事件 - 关闭监听socket
     */
    void handleClose() override;
    
    /**
     * @brief Server只关心READ事件（accept）
     */
    int interest() const override { return static_cast<int>(EventType::READ); }
    
    /**
     * @brief Server不需要发送数据
     */
    bool hasDataToSend() const override { return false; }

private:
    /**
     * @brief 创建并配置监听socket
     * 
     * 内部流程：
     * 1. socket()创建TCP socket
     * 2. setsockopt(SO_REUSEADDR)允许地址复用
     * 3. fcntl(O_NONBLOCK)设为非阻塞
     * 4. bind()绑定端口
     * 5. listen()开始监听
     */
    void setupServerSocket();

    uint16_t m_port;              // 监听端口
    int m_serverSocket;            // 监听socket fd
    sockaddr_in m_serverAddr;      // 服务器地址
    Reactor* m_reactor;            // 所属Reactor指针
};

#endif // SERVER_H
