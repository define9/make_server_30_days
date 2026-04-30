#include "Connection.h"
#include "log/Logger.h"
#include "socket/SocketOptions.h"
#include "util/MimeType.h"
#include "util/FileUtil.h"
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
    , m_httpParser()
    , m_keepAlive(true)
    , m_responseComplete(false)
{
    SocketOptions::ClientOptions options;
    options.tcpNoDelay = true;
    options.keepAlive = true;
    options.nonBlock = true;
    SocketOptions::applyClientOptions(m_fd, options);
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
    , m_httpParser(std::move(other.m_httpParser))
    , m_keepAlive(other.m_keepAlive)
    , m_responseComplete(other.m_responseComplete)
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
        m_httpParser = std::move(other.m_httpParser);
        m_keepAlive = other.m_keepAlive;
        m_responseComplete = other.m_responseComplete;
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
            break;
        }

        m_readBuffer.append(buffer, bytesRead);
        m_lastActivity = currentTimeMs();
    }

    if (m_readBuffer.empty())
        return true;

    LOG_DEBUG("Worker %d read %zu bytes from %s", m_workerId, m_readBuffer.size(), getClientInfo().c_str());
    size_t consumed = m_httpParser.parse(m_readBuffer.data(), m_readBuffer.size());

    if (consumed > 0) {
        m_readBuffer.erase(0, consumed);
    }

    if (m_httpParser.hasCompleteRequest()) {
        if (!processHttpRequest()) {
            return false;
        }
        if (!m_keepAlive) {
            m_state = ConnState::CLOSING;
            return true;
        }
        m_httpParser.reset();
    }

    return true;
}

bool Connection::processHttpRequest()
{
    const HttpRequest& request = m_httpParser.request();
    
    LOG_INFO("HTTP %s %s from %s", 
             request.methodString().c_str(), 
             request.url().c_str(),
             getClientInfo().c_str());

    m_keepAlive = request.keepAlive();
    buildHttpResponse(request);
    return true;
}

void Connection::buildHttpResponse(const HttpRequest& request)
{
    HttpResponse response;

    if (request.method() == HttpMethod::GET || request.method() == HttpMethod::POST) {
        if (request.path().find("/static/") == 0) {
            std::string filePath = request.path().substr(1);
            std::string fileContent;
            if (FileUtil::readFile(filePath, fileContent)) {
                response.setStatus(HttpStatusCode::OK);
                response.setContentType(MimeType::get(filePath));
                response.setKeepAlive(request.keepAlive());
                response.setBody(fileContent);
                m_writeBuffer = response.build();
                LOG_DEBUG("Worker %d serving static file: %s (%zu bytes)", m_workerId, filePath.c_str(), fileContent.size());
                return;
            } else {
                response.setStatus(HttpStatusCode::NOT_FOUND);
                response.setContentType("text/plain");
                response.setBody("404 Not Found: " + request.path() + "\n");
                response.setKeepAlive(request.keepAlive());
                m_writeBuffer = response.build();
                return;
            }
        }

        if (SimpleRouter::instance().route(request, response)) {
            m_writeBuffer = response.build();
            return;
        }

        response.setStatus(HttpStatusCode::NOT_FOUND);
        response.setContentType("text/plain");
        response.setBody("404 Not Found: " + request.path() + "\n");
        response.setKeepAlive(request.keepAlive());
        m_writeBuffer = response.build();
    } else {
        response.setStatus(HttpStatusCode::METHOD_NOT_ALLOWED);
        response.setBody("Method Not Allowed\n");
        response.setKeepAlive(false);
        m_writeBuffer = response.build();
    }
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

    if (m_writeBuffer.empty()) {
        if (!m_keepAlive) {
            m_state = ConnState::CLOSING;
        }
    } else {
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