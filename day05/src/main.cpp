/**
 * @file main.cpp
 * @brief 程序入口 - Day05
 *
 * Day05变化：
 * - EventLoop使用epoll ET模式
 * - 非阻塞I/O，循环读写直到EAGAIN
 * - Linux特有
 */

#include "Server.h"
#include "EventLoop.h"
#include <iostream>
#include <csignal>

// 全局指针，用于信号处理
static EventLoop* g_loop = nullptr;

// 信号处理
void signalHandler(int)
{
    if (g_loop) {
        g_loop->stop();
    }
}

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " <port>" << std::endl;
    std::cout << "Example: " << programName << " 8080" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    if (port == 0) {
        std::cerr << "Error: Invalid port" << std::endl;
        return 1;
    }

    try {
        // 创建EventLoop
        EventLoop loop;

        // 创建Server，传入EventLoop
        Server server(port, &loop);

        // 设置信号处理
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        g_loop = &loop;

        // 启动事件循环
        loop.loop(server.fd());

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server stopped" << std::endl;
    return 0;
}
