/**
 * @file Connection.h
 * @brief 客户端连接封装
 *
 * 每个已连接的客户端对应一个Connection对象
 * 管理客户端的socket、数据缓冲区和状态
 *
 * Day04无变化：Connection层与I/O多路复用方式无关
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

/**
 * @brief 连接状态
 */
enum class ConnState {
    CONNECTED,   // 已连接
    CLOSING,     // 正在关闭
    CLOSED       // 已关闭
};

/**
 * @brief Connection类 - 封装客户端连接
 *
 * 职责：
 * 1. 管理客户端socket
 * 2. 读写数据
 * 3. 保存客户端地址
 */
class Connection {
public:
    /**
     * @brief 构造函数
     * @param fd 客户端socket描述符
     * @param addr 客户端地址
     */
    Connection(int fd, const sockaddr_in& addr);

    /**
     * @brief 析构函数
     */
    ~Connection();

    // 禁用拷贝
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    // 移动语义
    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    /**
     * @brief 处理读事件
     * @return 成功返回true，连接断开返回false
     */
    bool handleRead();

    /**
     * @brief 处理写事件
     * @return 成功返回true，连接断开返回false
     */
    bool handleWrite();

    /**
     * @brief 关闭连接
     */
    void close();

    /**
     * @brief 获取fd
     */
    int fd() const { return m_fd; }

    /**
     * @brief 获取状态
     */
    ConnState state() const { return m_state; }

    /**
     * @brief 获取客户端信息
     */
    std::string getClientInfo() const;

    /**
     * @brief 是否可关闭
     */
    bool isClosed() const { return m_state == ConnState::CLOSED; }

private:
    int m_fd;                  // socket描述符
    sockaddr_in m_addr;        // 客户端地址
    ConnState m_state;         // 连接状态

    // 缓冲区：简单实现，实际项目用RingBuffer
    std::string m_readBuffer;  // 读缓冲区
    std::string m_writeBuffer; // 写缓冲区(echo时使用)
};

#endif // CONNECTION_H
