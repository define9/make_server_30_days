/**
 * @file SocketOptions.h
 * @brief Socket选项配置 - 网络领域
 */

#ifndef SOCKET_OPTIONS_H
#define SOCKET_OPTIONS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

/**
 * @brief Socket选项配置类
 * 
 * 统一管理socket的各种选项，包括：
 * - SO_REUSEADDR: 地址复用
 * - SO_REUSEPORT: 端口复用（需要OS支持）
 * - TCP_NODELAY: 禁用Nagle算法，降低延迟
 * - SO_KEEPALIVE: TCP保活
 * - SO_SNDBUF/SO_RCVBUF: 缓冲区大小
 * - SO_LINGER: 优雅关闭
 */
class SocketOptions {
public:
    /**
     * @brief 服务器socket选项配置
     */
    struct ServerOptions {
        bool reuseAddr = true;       // SO_REUSEADDR
        bool reusePort = false;       // SO_REUSEPORT (需要内核3.9+)
        int sendBuffer = 0;           // 0表示使用默认值
        int recvBuffer = 0;           // 0表示使用默认值
        bool nonBlock = true;         // 非阻塞模式
    };

    /**
     * @brief 客户端socket选项配置
     */
    struct ClientOptions {
        bool tcpNoDelay = true;       // TCP_NODELAY - 禁用Nagle算法
        bool keepAlive = false;       // SO_KEEPALIVE - TCP保活
        int sendBuffer = 0;           // 0表示使用默认值
        int recvBuffer = 0;           // 0表示使用默认值
        bool nonBlock = true;         // 非阻塞模式
        int lingerTimeout = 0;        // SO_LINGER超时，0表示优雅关闭
    };

    /**
     * @brief 应用选项到服务器socket
     * @param sockFd socket文件描述符
     * @param options 服务器选项
     * @return 是否全部成功
     */
    static bool applyServerOptions(int sockFd, const ServerOptions& options) {
        bool allSuccess = true;

        // SO_REUSEADDR - 地址复用
        if (options.reuseAddr) {
            int opt = 1;
            if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                return false;
            }
        }

        // SO_REUSEPORT - 端口复用 (Linux 3.9+)
        if (options.reusePort) {
            int opt = 1;
            if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
                // 忽略错误，因为旧内核可能不支持
            }
        }

        // SO_SNDBUF - 发送缓冲区
        if (options.sendBuffer > 0) {
            if (setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &options.sendBuffer, sizeof(options.sendBuffer)) < 0) {
                allSuccess = false;
            }
        }

        // SO_RCVBUF - 接收缓冲区
        if (options.recvBuffer > 0) {
            if (setsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &options.recvBuffer, sizeof(options.recvBuffer)) < 0) {
                allSuccess = false;
            }
        }

        // 非阻塞模式
        if (options.nonBlock) {
            int flags = fcntl(sockFd, F_GETFL, 0);
            if (flags < 0) {
                return false;
            }
            if (fcntl(sockFd, F_SETFL, flags | O_NONBLOCK) < 0) {
                return false;
            }
        }

        return allSuccess;
    }

    /**
     * @brief 应用选项到客户端socket
     * @param sockFd socket文件描述符
     * @param options 客户端选项
     * @return 是否全部成功
     */
    static bool applyClientOptions(int sockFd, const ClientOptions& options) {
        bool allSuccess = true;

        // TCP_NODELAY - 禁用Nagle算法
        if (options.tcpNoDelay) {
            int opt = 1;
            if (setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
                allSuccess = false;
            }
        }

        // SO_KEEPALIVE - TCP保活
        if (options.keepAlive) {
            int opt = 1;
            if (setsockopt(sockFd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
                allSuccess = false;
            }
        }

        // SO_SNDBUF - 发送缓冲区
        if (options.sendBuffer > 0) {
            if (setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &options.sendBuffer, sizeof(options.sendBuffer)) < 0) {
                allSuccess = false;
            }
        }

        // SO_RCVBUF - 接收缓冲区
        if (options.recvBuffer > 0) {
            if (setsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &options.recvBuffer, sizeof(options.recvBuffer)) < 0) {
                allSuccess = false;
            }
        }

        // SO_LINGER - 优雅关闭
        if (options.lingerTimeout >= 0) {
            struct linger ling;
            ling.l_onoff = (options.lingerTimeout > 0) ? 1 : 0;
            ling.l_linger = options.lingerTimeout;
            if (setsockopt(sockFd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) < 0) {
                allSuccess = false;
            }
        }

        // 非阻塞模式
        if (options.nonBlock) {
            int flags = fcntl(sockFd, F_GETFL, 0);
            if (flags < 0) {
                return false;
            }
            if (fcntl(sockFd, F_SETFL, flags | O_NONBLOCK) < 0) {
                return false;
            }
        }

        return allSuccess;
    }

    /**
     * @brief 获取socket错误状态
     */
    static bool hasError(int sockFd) {
        int opt;
        socklen_t optLen = sizeof(opt);
        if (getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &opt, &optLen) < 0) {
            return true;
        }
        return opt != 0;
    }

    /**
     * @brief 获取当前socket接收缓冲区大小
     */
    static int getRecvBufferSize(int sockFd) {
        int opt;
        socklen_t optLen = sizeof(opt);
        if (getsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &opt, &optLen) < 0) {
            return -1;
        }
        return opt;
    }

    /**
     * @brief 获取当前socket发送缓冲区大小
     */
    static int getSendBufferSize(int sockFd) {
        int opt;
        socklen_t optLen = sizeof(opt);
        if (getsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &opt, &optLen) < 0) {
            return -1;
        }
        return opt;
    }
};

#endif // SOCKET_OPTIONS_H