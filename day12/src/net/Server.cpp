/**
 * @file Server.cpp
 * @brief Server实现 - TCP服务器
 */

#include "Server.h"
#include "reactor/Reactor.h"
#include "Connection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

Server::Server(uint16_t port, Reactor* reactor, NextWorkerCallback callback)
    : m_port(port)
    , m_serverSocket(-1)
    , m_reactor(reactor)
    , m_nextWorker(callback)
{
    bzero(&m_serverAddr, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serverAddr.sin_port = htons(m_port);

    setupServerSocket();
    m_reactor->registerHandler(this);

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
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int flags = fcntl(m_serverSocket, F_GETFL, 0);
    fcntl(m_serverSocket, F_SETFL, flags | O_NONBLOCK);

    if (bind(m_serverSocket, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(m_serverSocket, 5) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to listen");
    }
}

bool Server::handleRead()
{
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int acceptCount = 0;

    while (true) {
        int clientSocket = accept(m_serverSocket, 
                                   (struct sockaddr*)&clientAddr, 
                                   &clientAddrLen);

        if (clientSocket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[Server] accept() failed: " << strerror(errno) << std::endl;
            break;
        }

        acceptCount++;
        std::cout << "[Server] Accepted client: "
                  << inet_ntoa(clientAddr.sin_addr) << ":"
                  << ntohs(clientAddr.sin_port)
                  << " (fd=" << clientSocket << ")" << std::endl;

        Connection* conn = new Connection(clientSocket, clientAddr);
        
        // 获取 work reactor ,把 conn 注册进去
        if (m_nextWorker) {
            Reactor* worker = m_nextWorker();
            if (worker) {
                worker->registerHandler(conn);
                worker->incrementConnections();
                conn->setWorkerId(worker->workerId());
                std::cout << "[Server] -> Worker " << worker->workerId() << std::endl;
            }
        } else {
            m_reactor->registerHandler(conn);
            m_reactor->incrementConnections();
        }
    }

    if (acceptCount > 1) {
        std::cout << "[Server] Accepted " << acceptCount << " connections in one batch" << std::endl;
    }

    return true;
}

void Server::handleClose()
{
    std::cout << "[Server] handleClose " << m_serverSocket << std::endl;
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
        m_serverSocket = -1;
    }
}