/**
 * @file Connection.cpp
 * @brief Connection实现
 */

#include "Connection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

static const size_t BUFFER_SIZE = 1024;

Connection::Connection(int fd, const sockaddr_in& addr)
    : m_fd(fd)
    , m_addr(addr)
    , m_state(ConnState::CONNECTED)
{
}

Connection::~Connection()
{
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

// 移动构造函数
Connection::Connection(Connection&& other) noexcept
    : m_fd(other.m_fd)
    , m_addr(other.m_addr)
    , m_state(other.m_state)
    , m_readBuffer(std::move(other.m_readBuffer))
    , m_writeBuffer(std::move(other.m_writeBuffer))
{
    other.m_fd = -1;
}

// 移动赋值运算符
Connection& Connection::operator=(Connection&& other) noexcept
{
    if (this != &other) {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
        m_fd = other.m_fd;
        m_addr = other.m_addr;
        m_state = other.m_state;
        m_readBuffer = std::move(other.m_readBuffer);
        m_writeBuffer = std::move(other.m_writeBuffer);
        other.m_fd = -1;
    }
    return *this;
}

bool Connection::handleRead()
{
    char buffer[BUFFER_SIZE];

    ssize_t bytesRead = ::read(m_fd, buffer, sizeof(buffer) - 1);

    if (bytesRead < 0) {
        if (errno == EINTR) {
            return true;  // 被信号打断，重试
        }
        std::cerr << "[Connection] read() error: " << strerror(errno) << std::endl;
        return false;
    }

    if (bytesRead == 0) {
        // 对端关闭连接
        std::cout << "[Connection] Client disconnected: " << getClientInfo() << std::endl;
        return false;
    }

    buffer[bytesRead] = '\0';
    m_readBuffer.append(buffer, bytesRead);

    std::cout << "[Connection] Read " << bytesRead << " bytes from " 
              << getClientInfo() << ": " << buffer << std::endl;

    return true;
}

bool Connection::handleWrite()
{
    if (m_writeBuffer.empty()) {
        return true;
    }

    ssize_t bytesWritten = ::write(m_fd, m_writeBuffer.c_str(), m_writeBuffer.size());

    if (bytesWritten < 0) {
        if (errno == EINTR) {
            return true;  // 被信号打断，重试
        }
        std::cerr << "[Connection] write() error: " << strerror(errno) << std::endl;
        return false;
    }

    if (bytesWritten > 0) {
        m_writeBuffer.erase(0, bytesWritten);
        std::cout << "[Connection] Wrote " << bytesWritten << " bytes to " 
                  << getClientInfo() << std::endl;
    }

    return true;
}

void Connection::close()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
        m_state = ConnState::CLOSED;
    }
}

std::string Connection::getClientInfo() const
{
    char addrStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &m_addr.sin_addr, addrStr, sizeof(addrStr));
    return std::string(addrStr) + ":" + std::to_string(ntohs(m_addr.sin_port));
}
