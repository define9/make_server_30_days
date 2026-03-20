/**
 * @file Connection.cpp
 * @brief Connection实现 - Day07 Reactor版本
 * 
 * 本文件实现Reactor模式中的Connection事件处理器。
 */

#include "Connection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

// 每次read/write操作的缓冲区大小
// 4096是一个常见的选择：刚好是页大小的整数倍，且足够大以容纳典型TCP数据包
static const size_t BUFFER_SIZE = 4096;

/**
 * @brief 构造函数
 * 
 * 关键操作：将fd设置为非阻塞模式
 * 
 * 为什么必须非阻塞？
 * - Reactor使用epoll ET(边缘触发)模式
 * - ET模式下，只在状态变化时通知一次
 * - 如果fd是阻塞的，处理过程中可能无限期等待
 * - 非阻塞I/O配合循环处理，确保不会阻塞事件循环
 */
Connection::Connection(int fd, const sockaddr_in& addr)
    : m_fd(fd)
    , m_addr(addr)
    , m_state(ConnState::CONNECTED)
{
    // 获取当前flags
    int flags = fcntl(m_fd, F_GETFL, 0);
    // 添加O_NONBLOCK标志，保持其他标志不变
    fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief 析构函数 - 确保fd被关闭
 * 
 * 资源管理策略：
 * - Connection拥有fd资源
 * - 析构时确保关闭fd，避免fd泄漏
 * - fd=-1表示已关闭，用于移动操作后
 */
Connection::~Connection()
{
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

/**
 * @brief 移动构造函数
 * 
 * 为什么需要移动语义？
 * - Connection拥有fd和缓冲区资源
 * - 标准容器的emplace等操作需要移动语义
 * - 避免不必要的拷贝（缓冲区可能很大）
 * 
 * 移动后：原对象的fd被置为-1，表示不再拥有资源
 */
Connection::Connection(Connection&& other) noexcept
    : m_fd(other.m_fd)
    , m_addr(other.m_addr)
    , m_state(other.m_state)
    , m_readBuffer(std::move(other.m_readBuffer))
    , m_writeBuffer(std::move(other.m_writeBuffer))
{
    // 原对象不再拥有fd
    other.m_fd = -1;
}

/**
 * @brief 移动赋值运算符
 * 
 * 自赋值检查：
 * - 移动操作可能发生在自赋值时
 * - 先关闭已有的fd，避免fd泄漏
 */
Connection& Connection::operator=(Connection&& other) noexcept
{
    if (this != &other) {
        // 先关闭自己的fd（如果有）
        if (m_fd >= 0) {
            ::close(m_fd);
        }
        // 移动所有资源
        m_fd = other.m_fd;
        m_addr = other.m_addr;
        m_state = other.m_state;
        m_readBuffer = std::move(other.m_readBuffer);
        m_writeBuffer = std::move(other.m_writeBuffer);
        // 原对象置为无效状态
        other.m_fd = -1;
    }
    return *this;
}

/**
 * @brief 返回当前感兴趣的事件类型
 * 
 * 动态事件注册策略：
 * - 无数据待发送：只关心READ（等待客户端数据）
 * - 有数据待发送：同时关心READ和WRITE（既要收也要发）
 * 
 * 为什么这样设计？
 * - 减少不必要的事件通知（WRITE事件可能频繁触发但无数据可写）
 * - 只有在有数据时才注册WRITE事件，更高效
 */
int Connection::interest() const
{
    if (m_writeBuffer.empty()) {
        // 无数据发送，只关心读事件
        return static_cast<int>(EventType::READ);
    }
    // 有数据待发送，同时关心读写
    return static_cast<int>(EventType::READ) | static_cast<int>(EventType::WRITE);
}

/**
 * @brief 处理读事件
 * 
 * 非阻塞读取流程（配合ET模式）：
 * 1. 循环调用read()直到返回EAGAIN/EWOULDBLOCK（数据读完）
 * 2. EINTR表示被信号中断，继续重试
 * 3. bytesRead==0表示客户端关闭连接
 * 4. 正常读取：将数据追加到m_readBuffer
 * 
 * ET模式注意事项：
 * - read()返回EAGAIN时，表示该fd已无更多数据
 * - 必须退出循环，等待下次事件通知
 * 
 * @return 成功返回true，客户端断开或错误返回false
 */
bool Connection::handleRead()
{
    char buffer[BUFFER_SIZE];
    bool hasData = false;

    // 循环读取，直到遇到EAGAIN（数据读完）或错误
    while (true) {
        ssize_t bytesRead = ::read(m_fd, buffer, sizeof(buffer));

        if (bytesRead < 0) {
            // EINTR：系统调用被信号中断，可恢复
            if (errno == EINTR) {
                continue;
            }
            // EAGAIN/EWOULDBLOCK：表示该fd暂时无数据可读（ET模式正常结束）
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            // 其他错误：打印日志并通知Reactor关闭连接
            std::cerr << "[Connection] read() error: " << strerror(errno) << std::endl;
            return false;
        }

        // bytesRead == 0：客户端关闭了连接（收到FIN）
        if (bytesRead == 0) {
            std::cout << "[Connection] Client disconnected: " << getClientInfo() << std::endl;
            return false;
        }

        hasData = true;
        // 将读取的数据追加到读缓冲区
        m_readBuffer.append(buffer, bytesRead);
    }

    // 处理读取到的数据（Echo服务器：直接回显）
    if (hasData) {
        std::cout << "[Connection] Read total " << m_readBuffer.size() << " bytes from "
                  << getClientInfo() << ": " << m_readBuffer << std::endl;

        // 将读取的数据放入写缓冲区，等待handleWrite发送
        m_writeBuffer.append(m_readBuffer);
        m_readBuffer.clear();
    }

    return true;
}

/**
 * @brief 处理写事件
 * 
 * 非阻塞发送流程：
 * 1. 循环调用write()直到缓冲区数据全部发送或遇到EAGAIN
 * 2. EAGAIN表示发送缓冲区已满（TCP滑动窗口为0）
 * 3. 部分写入时，删除已发送的部分，继续发送剩余数据
 * 
 * 为什么需要循环？
 * - 非阻塞write()可能只发送部分数据
 * - 必须循环直到全部发送或遇到阻塞
 * 
 * @return 成功返回true，错误返回false
 */
bool Connection::handleWrite()
{
    // 循环发送，直到缓冲区空或遇到EAGAIN
    while (!m_writeBuffer.empty()) {
        ssize_t bytesWritten = ::write(m_fd, m_writeBuffer.c_str(), m_writeBuffer.size());

        if (bytesWritten < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 发送缓冲区满，下次继续
                break;
            }
            std::cerr << "[Connection] write() error: " << strerror(errno) << std::endl;
            return false;
        }

        // 成功写入：从缓冲区删除已发送的部分
        if (bytesWritten > 0) {
            m_writeBuffer.erase(0, bytesWritten);
        }
    }

    // 如果还有剩余数据未发送完，说明遇到了EAGAIN
    if (!m_writeBuffer.empty()) {
        std::cout << "[Connection] Remaining " << m_writeBuffer.size()
                  << " bytes to write to " << getClientInfo() << std::endl;
    }

    return true;
}

/**
 * @brief 处理关闭事件
 * 
 * Reactor调用此方法通知Handler关闭连接。
 * 通常在检测到错误或客户端断开时调用。
 * 
 * 注意：调用后Reactor会自动调用removeHandler()移除该Handler
 */
void Connection::handleClose()
{
    std::cout << "[Connection] Closing: " << getClientInfo() << std::endl;
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;              // 标记为无效，避免析构时重复关闭
        m_state = ConnState::CLOSED;
    }
}

/**
 * @brief 获取客户端地址信息
 * @return "IP:Port"格式字符串
 * 
 * 用于日志和调试：
 * - 方便追踪是哪个客户端在做操作
 * - inet_ntop将二进制IP转换为可读字符串
 * - ntohs将网络字节序端口转换为主机字节序
 */
std::string Connection::getClientInfo() const
{
    char addrStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &m_addr.sin_addr, addrStr, sizeof(addrStr));
    return std::string(addrStr) + ":" + std::to_string(ntohs(m_addr.sin_port));
}
