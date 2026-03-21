#include "Connection.h"
#include "log/Logger.h"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>

static const size_t BUFFER_SIZE = 4096;

static int64_t currentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

Connection::Connection(int fd, const sockaddr_in& addr)
    : m_fd(fd)
    , m_addr(addr)
    , m_state(ConnState::CONNECTED)
    , m_workerId(-1)
    , m_lastActivity(currentTimeMs())
    , m_timerId(-1)
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
    , m_workerId(other.m_workerId)
    , m_lastActivity(other.m_lastActivity)
    , m_timerId(other.m_timerId)
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
        m_workerId = other.m_workerId;
        m_lastActivity = other.m_lastActivity;
        m_timerId = other.m_timerId;
        other.m_fd = -1;
    }
    return *this;
}

int Connection::interest() const
{
    if (m_writeBuffer.empty()) {
        return static_cast<int>(EventType::READ);
    }
    return static_cast<int>(EventType::READ) | static_cast<int>(EventType::WRITE);
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
            LOG_ERROR("[Connection] read() error: %s", strerror(errno));
            return false;
        }

        if (bytesRead == 0) {
            LOG_INFO("Client disconnected: %s", getClientInfo().c_str());
            return false;
        }

        hasData = true;
        m_readBuffer.append(buffer, bytesRead);
    }

    if (hasData) {
        LOG_DEBUG("Worker %d read %zu bytes from %s: %s", 
                  m_workerId, m_readBuffer.size(), getClientInfo().c_str(), m_readBuffer.c_str());

        m_writeBuffer.append(m_readBuffer);
        m_readBuffer.clear();
        m_lastActivity = currentTimeMs();
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
            LOG_ERROR("[Connection] write() error: %s", strerror(errno));
            return false;
        }

        if (bytesWritten > 0) {
            m_writeBuffer.erase(0, bytesWritten);
        }
    }

    if (!m_writeBuffer.empty()) {
        LOG_DEBUG("Worker %d remaining %zu bytes to %s", 
                  m_workerId, m_writeBuffer.size(), getClientInfo().c_str());
    }

    return true;
}

void Connection::handleClose()
{
    LOG_DEBUG("Worker %d closing: %s", m_workerId, getClientInfo().c_str());
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