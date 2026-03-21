/**
 * @file ClientHandler.cpp
 * @brief 客户端处理器实现
 */

#include "ClientHandler.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

// 缓冲区大小
static const size_t BUFFER_SIZE = 1024;

ClientHandler::ClientHandler(int clientSocket, const sockaddr_in& clientAddr)
    : m_clientSocket(clientSocket)
    , m_clientAddr(clientAddr)
    , m_running(false)
{
}

ClientHandler::~ClientHandler()
{
    // 析构时确保socket被关闭
    if (m_clientSocket >= 0) {
        close(m_clientSocket);
    }
}

void ClientHandler::handle()
{
    m_running = true;
    std::cout << "[Handler] Client connected: " << getClientInfo() << std::endl;

    // 循环处理客户端数据，直到客户端断开
    while (m_running) {
        if (!readAndEcho()) {
            break;
        }
    }

    std::cout << "[Handler] Client disconnected: " << getClientInfo() << std::endl;
}

bool ClientHandler::readAndEcho()
{
    // 读取缓冲区
    char buffer[BUFFER_SIZE];

    // read() 从socket读取数据
    // 返回值:
    //   > 0: 读取到的字节数
    //   = 0: 对端关闭了连接 (read返回0表示EOF)
    //   < 0: 出错 (可能是被打断，也可能是真的错误)
    ssize_t bytesRead = read(m_clientSocket, buffer, sizeof(buffer) - 1);

    if (bytesRead < 0) {
        // 如果被信号打断，可以重试
        // EINTR表示被信号中断
        if (errno == EINTR) {
            return true;
        }
        std::cerr << "[Handler] read() error for " << getClientInfo() << std::endl;
        return false;
    }

    if (bytesRead == 0) {
        // 客户端关闭了连接
        return false;
    }

    // 添加字符串结束符，方便打印
    buffer[bytesRead] = '\0';

    // 打印收到的消息
    std::cout << "[Handler] Received (" << bytesRead << " bytes): " << buffer << std::endl;

    // 简单的echo：把收到的数据原样发回去
    // 注意：这只是echo，实际服务器可能会有其他处理逻辑
    // 使用send()替代write()，配合MSG_NOSIGNAL避免SIGPIPE
    ssize_t bytesWritten = send(m_clientSocket, buffer, bytesRead, MSG_NOSIGNAL);

    if (bytesWritten < 0) {
        // EPIPE表示对端已关闭连接，不是错误而是正常断开
        if (errno == EPIPE) {
            std::cout << "[Handler] Client closed connection (EPIPE): " << getClientInfo() << std::endl;
            return false;
        }
        std::cerr << "[Handler] send() error for " << getClientInfo() << std::endl;
        return false;
    }

    std::cout << "[Handler] Echo: " << buffer << std::endl;

    return true;
}

std::string ClientHandler::getClientInfo() const
{
    char addrStr[INET_ADDRSTRLEN];

    // inet_ntop() 是inet_ntoa()的现代版本，支持IPv6
    // INET_ADDRSTRLEN足够容纳IPv4地址
    inet_ntop(AF_INET, &m_clientAddr.sin_addr, addrStr, sizeof(addrStr));

    // 格式: IP:Port
    return std::string(addrStr) + ":" + std::to_string(ntohs(m_clientAddr.sin_port));
}
