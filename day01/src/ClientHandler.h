/**
 * @file ClientHandler.h
 * @brief 客户端处理器类 - 负责与单个客户端的通信
 *
 * 每次服务器接受一个新客户端连接，就创建一个ClientHandler对象。
 * 该对象负责处理与这个客户端的所有通信，直到客户端断开连接。
 */

#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

/**
 * @brief 客户端处理器类
 *
 * 职责：
 * 1. 管理客户端socket生命周期
 * 2. 读取客户端数据
 * 3. Echo响应（将数据返回给客户端）
 * 4. 检测客户端断开
 *
 * 设计考虑：
 * - 每个ClientHandler对应一个客户端
 * - 处理逻辑在handle()方法中
 * - 使用阻塞I/O，简单可靠
 */
class ClientHandler {
public:
    /**
     * @brief 构造函数
     * @param clientSocket 已连接的客户端socket描述符
     * @param clientAddr 客户端的地址信息
     */
    ClientHandler(int clientSocket, const sockaddr_in& clientAddr);

    /**
     * @brief 析构函数
     *
     * 关闭客户端socket
     */
    ~ClientHandler();

    // 禁用拷贝
    ClientHandler(const ClientHandler&) = delete;
    ClientHandler& operator=(const ClientHandler&) = delete;

    /**
     * @brief 处理客户端请求
     *
     * 在一个循环中：
     * 1. 读取客户端发送的数据
     * 2. 如果有数据，将数据echo回去
     * 3. 如果客户端关闭连接，退出循环
     *
     * 这是阻塞操作，会一直处理直到客户端断开。
     */
    void handle();

private:
    /**
     * @brief 读取并echo数据
     * @return 成功返回true，客户端断开返回false
     */
    bool readAndEcho();

    /**
     * @brief 获取客户端地址字符串
     * @return IP地址:端口号 格式的字符串
     */
    std::string getClientInfo() const;

    int m_clientSocket;           ///< 客户端socket描述符
    sockaddr_in m_clientAddr;     ///< 客户端地址
    bool m_running;               ///< 处理状态
};

#endif // CLIENT_HANDLER_H
