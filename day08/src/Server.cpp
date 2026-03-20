/**
 * @file Server.cpp
 * @brief Server实现 - Day07 Reactor版本
 * 
 * 本文件实现TCP服务器，作为Reactor模式中的事件源。
 */

#include "Server.h"
#include "Reactor.h"
#include "Connection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

/**
 * @brief 构造函数
 * 
 * 初始化服务器地址，然后创建socket并注册到Reactor。
 * 
 * @param port 监听端口
 * @param reactor 所属Reactor指针
 */
Server::Server(uint16_t port, Reactor* reactor)
    : m_port(port)
    , m_serverSocket(-1)
    , m_reactor(reactor)
{
    // 初始化地址结构为0
    bzero(&m_serverAddr, sizeof(m_serverAddr));
    
    // 配置地址结构
    m_serverAddr.sin_family = AF_INET;                    // IPv4
    m_serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);   // 监听所有网卡
    m_serverAddr.sin_port = htons(m_port);               // 端口（网络字节序）

    // 创建并配置socket
    setupServerSocket();
    
    // 将自己注册到Reactor（这样Server的accept事件会被Reactor分发）
    m_reactor->registerHandler(this);

    std::cout << "[Server] Server listening on port " << m_port << std::endl;
}

/**
 * @brief 析构函数
 */
Server::~Server()
{
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
    }
}

/**
 * @brief 创建并配置监听socket
 * 
 * socket创建流程：
 * 1. socket() - 创建TCP socket
 * 2. setsockopt(SO_REUSEADDR) - 允许地址复用（快速重启）
 * 3. fcntl(O_NONBLOCK) - 设为非阻塞（配合ET模式）
 * 4. bind() - 绑定地址和端口
 * 5. listen() - 开始监听
 * 
 * 为什么需要SO_REUSEADDR？
 * - 服务重启时 TIME_WAIT 状态的连接仍占用端口
 * - 有了这个选项，可以立即绑定同一端口
 * - 开发调试时特别有用
 * 
 * 为什么需要O_NONBLOCK？
 * - Reactor使用ET模式，必须非阻塞
 * - accept()在ET模式下必须循环调用直到EAGAIN
 * - 如果是阻塞的，可能长时间卡在accept()
 */
void Server::setupServerSocket()
{
    // 1. 创建TCP socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // 2. 设置SO_REUSEADDR（允许地址复用）
    int opt = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 设为非阻塞（配合epoll ET模式）
    int flags = fcntl(m_serverSocket, F_GETFL, 0);
    fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

    // 4. 绑定地址和端口
    if (bind(m_serverSocket, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    // 5. 开始监听（backlog=5）
    if (listen(m_serverSocket, 5) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to listen");
    }
}

/**
 * @brief 处理读事件 - accept新连接
 * 
 * 当epoll通知监听socket可读时，说明TCP backlog队列中有pending连接。
 * 此时应该循环调用accept()接受所有连接。
 * 
 * ET模式下的accept：
 * - 必须循环调用accept()直到返回EAGAIN
 * - 因为ET只通知一次，错过就要等下次
 * - 这也是为什么listen socket必须非阻塞
 * 
 * @return 总是返回true（Server不会因错误关闭）
 */
bool Server::handleRead()
{
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int acceptCount = 0;

    // 循环accept直到EAGAIN（ET模式必须这样做）
    while (true) {
        int clientSocket = accept(m_serverSocket, 
                                   (struct sockaddr*)&clientAddr, 
                                   &clientAddrLen);

        if (clientSocket < 0) {
            // EAGAIN/EWOULDBLOCK：没有更多连接了，退出循环
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            // EINTR：被信号中断，重试
            if (errno == EINTR) {
                continue;
            }
            // 其他错误：记录并退出
            std::cerr << "[Server] accept() failed: " << strerror(errno) << std::endl;
            break;
        }

        // 成功accept一个连接
        acceptCount++;
        std::cout << "[Server] Accepted client: "
                  << inet_ntoa(clientAddr.sin_addr) << ":"
                  << ntohs(clientAddr.sin_port)
                  << " (fd=" << clientSocket << ")" << std::endl;

        Connection* conn = new Connection(clientSocket, clientAddr);
        
        m_reactor->registerHandler(conn);
        m_reactor->incrementConnections();
    }

    // 如果一次触发accept了多个连接，打印日志（可能存在惊群效应）
    if (acceptCount > 1) {
        std::cout << "[Server] Accepted " << acceptCount << " connections in one batch" << std::endl;
    }

    return true;
}

/**
 * @brief 处理关闭事件
 * 
 * 当Server需要关闭时，清理监听socket资源。
 * 注意：调用后Reactor会移除对Server的引用。
 */
void Server::handleClose()
{
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
        m_serverSocket = -1;  // 标记为无效
    }
}