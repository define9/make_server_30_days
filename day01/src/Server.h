/**
 * @file Server.h
 * @brief TCP服务器类 - 负责端口监听和客户端连接接受
 *
 * 本类是整个服务器的核心，负责：
 * 1. 创建监听socket
 * 2. 绑定端口
 * 3. 监听连接
 * 4. 接受客户端连接
 *
 * 使用面向对象设计，将socket操作封装起来，提供简洁的接口。
 */

#ifndef SERVER_H
#define SERVER_H

#include <cstdint>      // uint16_t
#include <sys/socket.h> // sockaddr_in
#include <netinet/in.h>

/**
 * @brief TCP服务器类
 *
 * 封装TCP服务器的创建、配置、启动过程。
 * 使用RAII(Resource Acquisition Is Initialization)思想，
 * 在构造函数中初始化资源，析构函数中释放资源。
 */
class Server {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口号 (1-65535)
     * @throws std::runtime_error 如果端口无效或socket创建失败
     *
     * 示例:
     *   Server server(8080);  // 创建监听8080端口的服务器
     */
    Server(uint16_t port);

    /**
     * @brief 析构函数
     *
     * 关闭监听socket，释放资源
     */
    ~Server();

    // 禁用拷贝构造函数和赋值运算符，防止socket被重复关闭
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /**
     * @brief 启动服务器
     *
     * 开始监听端口，接受客户端连接。
     * 这个函数会阻塞，直到收到SIGINT/SIGTERM信号。
     *
     * 使用示例:
     *   Server server(8080);
     *   server.start();  // 阻塞运行
     */
    void start();

private:
    /**
     * @brief 创建并配置监听socket
     *
     * 主要步骤：
     * 1. socket() - 创建socket
     * 2. setsockopt() - 设置socket选项(地址复用)
     * 3. bind() - 绑定地址和端口
     * 4. listen() - 开始监听
     */
    void setupServerSocket();

    /**
     * @brief 接受客户端连接
     *
     * 调用accept()等待客户端连接。
     * 返回一个新的socket描述符用于与该客户端通信。
     *
     * @return 客户端socket描述符，失败返回-1
     */
    int acceptClient();

    /**
     * @brief 打印错误信息到标准错误
     * @param msg 错误消息前缀
     * @param errNum errno错误码，0表示使用当前的errno
     */
    void logError(const char* msg, int errNum = 0);

    uint16_t m_port;              ///< 监听端口
    int m_serverSocket;           ///< 服务器监听socket描述符
    sockaddr_in m_serverAddr;     ///< 服务器地址结构
    bool m_running;                ///< 服务器运行状态标志
};

#endif // SERVER_H
