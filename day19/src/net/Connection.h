#ifndef CONNECTION_H
#define CONNECTION_H

#include "Selector.h"
#include "reactor/EventHandler.h"
#include "http/HttpParser.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string>
#include <cstdint>

enum class ConnState {
    CONNECTED,
    CLOSING,
    CLOSED
};

class Connection : public EventHandler {
public:
    Connection(int fd, const sockaddr_in& addr);
    ~Connection();
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    int fd() const override { return m_fd; }
    bool handleRead() override;
    bool handleWrite() override;
    void handleClose() override;
    int interest() const override;
    bool hasDataToSend() const override { return !m_writeBuffer.empty(); }
    std::string getClientInfo() const;
    bool isClosed() const { return m_state == ConnState::CLOSED; }
    ConnState state() const { return m_state; }
    void setWorkerId(int id) { m_workerId = id; }
    int workerId() const { return m_workerId; }

    int64_t lastActivity() const { return m_lastActivity; }
    void updateActivity() { m_lastActivity = 0; }
    void setTimerId(int64_t id) { m_timerId = id; }
    int64_t timerId() const { return m_timerId; }

    // Keep-Alive support
    void setKeepAlive(bool keepAlive) { m_keepAlive = keepAlive; }
    bool keepAlive() const { return m_keepAlive; }
    bool isResponseComplete() const { return m_responseComplete; }
    void setResponseComplete(bool complete) { m_responseComplete = complete; }

private:
    bool processHttpRequest();
    void buildHttpResponse(const HttpRequest& request);

    int m_fd;
    sockaddr_in m_addr;
    ConnState m_state;
    std::string m_readBuffer;
    std::string m_writeBuffer;
    int m_workerId;
    int64_t m_lastActivity;
    int64_t m_timerId;
    HttpParser m_httpParser;
    bool m_keepAlive;
    bool m_responseComplete;
};

#endif