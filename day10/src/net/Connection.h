/**
 * @file Connection.h
 * @brief 客户端连接封装 - 网络领域
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "Selector.h"
#include "reactor/EventHandler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string>

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

private:
    int m_fd;
    sockaddr_in m_addr;
    ConnState m_state;
    std::string m_readBuffer;
    std::string m_writeBuffer;
    int m_workerId;
};

#endif // CONNECTION_H