/**
 * @file Connection.cpp
 * @brief Connection实现 - 非阻塞I/O版本
 *
 * ET模式下必须：
 * 1. socket设置为O_NONBLOCK
 * 2. 循环读写直到EAGAIN
 */

#include "Connection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

static const size_t BUFFER_SIZE = 4096;

Connection::Connection(int fd, const sockaddr_in& addr)
    : m_fd(fd)
    , m_addr(addr)
    , m_state(ConnState::CONNECTED)
{
    int flags = fcntl(m_fd, F_GETFL, 0);
    fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
}

Connection::~Connection()
{
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

Connection::Connection(Connection&& other) noexcept
    : m_fd(other.m_fd)
    , m_addr(other.m_addr)
    , m_state(other.m_state)
    , m_readBuffer(std::move(other.m_readBuffer))
    , m_writeBuffer(std::move(other.m_writeBuffer))
{
    other.m_fd = -1;
}

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
    bool hasData = false;

    while (true) {
        ssize_t bytesRead = ::read(m_fd, buffer, sizeof(buffer));

        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "[Connection] read() error: " << strerror(errno) << std::endl;
            return false;
        }

        if (bytesRead == 0) {
            std::cout << "[Connection] Client disconnected: " << getClientInfo() << std::endl;
            return false;
        }

        hasData = true;
        m_readBuffer.append(buffer, bytesRead);
    }

    if (hasData) {
        // echo
        std::cout << "[Connection] Read total " << m_readBuffer.size() << " bytes from "
                  << getClientInfo() << ": " << m_readBuffer << std::endl;

        m_writeBuffer.append(m_readBuffer);
        m_readBuffer.clear();
    }

    return true;
}

bool Connection::handleWrite()
{
    while (!m_writeBuffer.empty()) {
        ssize_t bytesWritten = ::write(m_fd, m_writeBuffer.c_str(), m_writeBuffer.size());

        if (bytesWritten < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "[Connection] write() error: " << strerror(errno) << std::endl;
            return false;
        }

        if (bytesWritten > 0) {
            m_writeBuffer.erase(0, bytesWritten);
        }
    }

    if (!m_writeBuffer.empty()) {
        std::cout << "[Connection] Remaining " << m_writeBuffer.size()
                  << " bytes to write to " << getClientInfo() << std::endl;
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
