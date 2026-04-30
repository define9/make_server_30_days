/**
 * @file Server.cpp
 * @brief Server实现 - TCP服务器
 */

#include "Server.h"
#include "Connection.h"
#include "log/Logger.h"
#include "socket/SocketOptions.h"
#include "reactor/Reactor.h"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

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

    LOG_INFO("Server listening on port %d", m_port);
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

    SocketOptions::ServerOptions options;
    options.reuseAddr = true;
    options.reusePort = false;
    options.nonBlock = true;

    if (!SocketOptions::applyServerOptions(m_serverSocket, options)) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to apply socket options");
    }

    if (bind(m_serverSocket, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(m_serverSocket, 5) < 0) {
        close(m_serverSocket);
        throw std::runtime_error("Failed to listen");
    }

    LOG_INFO("Server socket options: REUSEADDR=%s, REUSEPORT=%s, NONBLOCK=%s",
             options.reuseAddr ? "on" : "off",
             options.reusePort ? "on" : "off",
             options.nonBlock ? "on" : "off");
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
            LOG_ERROR("accept() failed: %s", strerror(errno));
            break;
        }

        acceptCount++;
        LOG_INFO("Accepted client: %s:%d (fd=%d)",
                 inet_ntoa(clientAddr.sin_addr),
                 ntohs(clientAddr.sin_port),
                 clientSocket);

        Connection* conn = new Connection(clientSocket, clientAddr);
        
        // 获取 work reactor ,把 conn 注册进去
        if (m_nextWorker) {
            Reactor* worker = m_nextWorker();
            if (worker) {
                worker->registerHandler(conn);
                worker->incrementConnections();
                conn->setWorkerId(worker->workerId());
                LOG_DEBUG("-> Worker %d", worker->workerId());
            }
        } else {
            m_reactor->registerHandler(conn);
            m_reactor->incrementConnections();
        }
    }

    if (acceptCount > 1) {
        LOG_DEBUG("Accepted %d connections in one batch", acceptCount);
    }

    return true;
}

void Server::handleClose()
{
    LOG_DEBUG("handleClose %d", m_serverSocket);
    if (m_serverSocket >= 0) {
        close(m_serverSocket);
        m_serverSocket = -1;
    }
}