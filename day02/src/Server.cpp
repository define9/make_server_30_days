/**
 * @file Server.cpp
 * @brief Server实现
 */

#include "Server.h"
#include "EventLoop.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

Server::Server(uint16_t port, EventLoop* loop)
    : m_port(port)
    , m_serverSocket(-1)
    , m_loop(loop)
{
    bzero(&m_serverAddr, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddr.sin_port = htons(m_port);

    setupServerSocket();

    // 设置EventLoop的新连接回调
    // 当EventLoop检测到serverFd可读时，调用acceptConnection
    m_loop->setNewConnectionCallback([this](int) {
        this->acceptConnection();
    });

    std::cout << "[Server] Server listening on port " << m_port << std::endl;
}

Server::~Server()
{
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
    }
}

void Server::setupServerSocket()
{
    // 创建socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // 地址复用
    int opt = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定
    if (bind(m_serverSocket, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    // 监听
    if (listen(m_serverSocket, 5) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to listen");
    }
}

void Server::acceptConnection()
{
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientSocket = accept(m_serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

    if (clientSocket < 0) {
        std::cerr << "[Server] accept() failed: " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "[Server] Accepted client: " 
              << inet_ntoa(clientAddr.sin_addr) << ":"
              << ntohs(clientAddr.sin_port) 
              << " (fd=" << clientSocket << ")" << std::endl;

    // 创建Connection并交给EventLoop管理
    m_loop->addConnection(clientSocket, clientAddr);
}
