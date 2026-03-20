/**
 * @file main.cpp
 * @brief 程序入口
 *
 * 简单的命令行入口，接受端口号参数
 */

#include "Server.h"
#include <iostream>
#include <cstdlib>

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " <port>" << std::endl;
    std::cout << "Example: " << programName << " 8080" << std::endl;
}

int main(int argc, char* argv[])
{
    // 检查命令行参数
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }

    // atoi() 将字符串转换为整数
    // 注意：没有错误检查，如果传入"abc"会返回0
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));

    // 简单的端口号验证
    if (port == 0) {
        std::cerr << "Error: Invalid port number" << std::endl;
        return 1;
    }

    try {
        // 创建并启动服务器
        Server server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
