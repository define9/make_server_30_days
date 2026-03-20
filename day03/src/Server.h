/**
 * @file Server.h
 * @brief TCP服务器 - Day03版本
 *
 * 相比Day01：
 * - 不再阻塞处理客户端
 * - 接受连接后交给EventLoop处理
 * - 提供新连接回调给EventLoop
 *
 * Day03无变化：Server层与I/O多路复用方式无关
 */

#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <functional>

class EventLoop;

/**
 * @brief TCP服务器类 - 接受连接
 *
 * 设计变化：
 * - 构造函数直接启动，不需要显式调用start()
 * - 通过回调函数将新连接交给EventLoop
 */
class Server {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param loop EventLoop指针，用于注册新连接
     */
    Server(uint16_t port, EventLoop* loop);

    /**
     * @brief 析构函数
     */
    ~Server();

    // 禁用拷贝
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /**
     * @brief 获取服务器socket
     */
    int fd() const { return m_serverSocket; }

private:
    /**
     * @brief 初始化服务器socket
     */
    void setupServerSocket();

    /**
     * @brief 处理新连接
     */
    void acceptConnection();

    uint16_t m_port;
    int m_serverSocket;
    sockaddr_in m_serverAddr;
    EventLoop* m_loop;
};

#endif // SERVER_H
