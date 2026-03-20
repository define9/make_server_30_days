/**
 * @file Server.h
 * @brief TCP服务器 - 多线程Reactor版本
 */

#ifndef SERVER_H
#define SERVER_H

#include "EventHandler.h"
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <functional>

class Reactor;

class Server : public EventHandler {
public:
    using NextWorkerCallback = std::function<Reactor*()>;
    
    Server(uint16_t port, Reactor* reactor, NextWorkerCallback callback);
    
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    int fd() const override { return m_serverSocket; }
    bool handleRead() override;
    bool handleWrite() override { return true; }
    void handleClose() override;
    int interest() const override { return static_cast<int>(EventType::READ); }
    bool hasDataToSend() const override { return false; }

private:
    void setupServerSocket();

    uint16_t m_port;
    int m_serverSocket;
    sockaddr_in m_serverAddr;
    Reactor* m_reactor;
    NextWorkerCallback m_nextWorker;
};

#endif // SERVER_H