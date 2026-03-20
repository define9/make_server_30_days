/**
 * @file Server.cpp
 * @brief TCP服务器实现
 */

#include "Server.h"
#include "ClientHandler.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

Server::Server(uint16_t port)
    : m_port(port)
    , m_serverSocket(-1)
    , m_running(false)
{
    // 初始化地址结构
    // bzero已废弃，但简单好用；生产环境可用memset
    bzero(&m_serverAddr, sizeof(m_serverAddr));

    // AF_INET: IPv4
    // AF_INET6: IPv6
    m_serverAddr.sin_family = AF_INET;

    // INADDR_ANY: 监听所有网卡
    // htonl: host to network long (字节序转换)
    m_serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // htons: host to network short (端口号字节序转换)
    // 注意：端口号需要字节序转换，IP地址不需要因为是32位
    m_serverAddr.sin_port = htons(m_port);

    // 创建并配置socket
    setupServerSocket();
}

Server::~Server()
{
    // 如果socket还在，关闭它
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
    }
}

void Server::setupServerSocket()
{
    // socket() 创建socket
    // AF_INET: IPv4协议族
    // SOCK_STREAM: 面向连接的可靠字节流 (TCP)
    // SOCK_DGRAM: 无连接的数据报 (UDP)
    // 0: 自动选择协议 (TCP时为0即可)
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        logError("socket() failed");
        throw std::runtime_error("Failed to create socket");
    }

    // setsockopt() 设置socket选项
    // SO_REUSEADDR: 地址复用，允许bind已使用的地址
    // 这对于服务器重启很重要，可以立即绑定刚关闭的端口
    int opt = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logError("setsockopt SO_REUSEADDR failed");
    }

    // bind() 绑定地址和端口
    // 将socket与特定的地址和端口关联
    // sockaddr_in需要转换为sockaddr* (通用socket地址结构)
    if (bind(m_serverSocket, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
        logError("bind() failed");
        close(m_serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    // listen() 开始监听
    // 第二个参数：backlog，accept队列的最大长度
    // 一般设置为5-10，太小可能拒绝连接，太大浪费内存
    if (listen(m_serverSocket, 5) < 0) {
        logError("listen() failed");
        close(m_serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "[Server] Server listening on port " << m_port << std::endl;
}

void Server::start()
{
    m_running = true;

    // 主循环：接受客户端连接
    // 这是阻塞I/O，accept()会一直等待直到有新连接
    while (m_running) {
        std::cout << "[Server] Waiting for client connection..." << std::endl;

        // 接受客户端连接
        int clientSocket = acceptClient();
        if (clientSocket < 0) {
            // accept失败，但服务器继续运行
            continue;
        }

        // 创建ClientHandler处理这个客户端
        // 这里阻塞处理一个客户端，处理完才接受下一个
        // 这是Day01的限制，明天会用select改进
        ClientHandler handler(clientSocket, m_serverAddr);
        handler.handle();
    }
}

int Server::acceptClient()
{
    // accept() 接受连接
    // 返回一个新的socket描述符，用于与该客户端通信
    // 原来的m_serverSocket继续监听，等待下一个连接

    // sizeof(clientAddr)可传入socklen_t*类型
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientSocket = accept(m_serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

    if (clientSocket < 0) {
        logError("accept() failed");
        return -1;
    }

    // 打印客户端信息
    // inet_ntoa() 将二进制IP地址转为点分十进制字符串
    // ntohs() 将端口号从网络字节序转回主机字节序
    std::cout << "[Server] Accepted client from "
              << inet_ntoa(clientAddr.sin_addr) << ":"
              << ntohs(clientAddr.sin_port) << std::endl;

    return clientSocket;
}

void Server::logError(const char* msg, int errNum)
{
    // 如果errNum为0，使用当前的errno
    int err = (errNum == 0) ? errno : errNum;

    // strerror() 将errno转为可读字符串
    std::cerr << "[Server] " << msg << ": " << strerror(err) << std::endl;
}
