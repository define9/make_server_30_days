/**
 * @file main.cpp
 * @brief 程序入口 - Day07 Reactor版本
 * 
 * ==================== 程序流程 ====================
 * 
 * 1. 解析命令行参数（端口号）
 * 2. 创建Reactor对象
 * 3. 创建Server对象（会自动注册到Reactor）
 * 4. 注册信号处理器（优雅退出）
 * 5. 启动Reactor事件循环
 * 6. 收到信号后优雅退出
 * 
 * ==================== Day07程序结构 ====================
 * 
 * main.cpp负责：
 * - 程序入口和参数解析
 * - 对象的创建和生命周期管理
 * - 信号处理（优雅退出）
 * 
 * 核心逻辑在Reactor.loop()中：
 * - 等待I/O事件
 * - 分发到Handler
 * - 处理错误和关闭
 */

#include "Server.h"
#include "Reactor.h"
#include <iostream>
#include <csignal>

/**
 * @brief 全局Reactor指针
 * 
 * 为什么需要全局变量？
 * - 信号处理函数不能传递参数
 * - 需要在信号处理函数中调用reactor.stop()
 * 
 * 注意：多线程环境下这是不安全的
 * 但单线程Reactor场景下可以接受
 */
static Reactor* g_reactor = nullptr;

/**
 * @brief 信号处理器
 * 
 * 捕获SIGINT和SIGTERM，实现优雅退出。
 * 
 * 为什么捕获这些信号？
 * - SIGINT (Ctrl+C)：用户主动中断
 * - SIGTERM (kill)：系统优雅终止信号
 * 
 * 处理逻辑：
 * - 设置全局Reactor的stop标志
 * - Reactor.loop()下次wait()返回后退出
 * 
 * @param sig 信号编号
 */
void signalHandler(int)
{
    if (g_reactor) {
        g_reactor->stop();
    }
}

/**
 * @brief 打印用法说明
 */
void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " <port>" << std::endl;
    std::cout << "Example: " << programName << " 8080" << std::endl;
}

/**
 * @brief 程序入口
 * 
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 *        argv[0] = 程序名
 *        argv[1] = 端口号
 * 
 * @return 0表示正常退出
 * 
 * 使用示例：
 * @code
 * ./server 8080
 * @endcode
 */
int main(int argc, char* argv[])
{
    // ===== 参数解析 =====
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }

    // 解析端口号
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    if (port == 0) {
        std::cerr << "Error: Invalid port" << std::endl;
        return 1;
    }

    try {
        // ===== 创建Reactor和Server =====
        Reactor reactor;
        Server server(port, &reactor);

        // ===== 注册信号处理器 =====
        // 捕获SIGINT（Ctrl+C）和SIGTERM（kill），实现优雅退出
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // 设置全局Reactor指针（供信号处理函数使用）
        g_reactor = &reactor;

        // ===== 启动事件循环 =====
        // 这是一个阻塞调用，直到stop()被调用才返回
        reactor.loop();

    } catch (const std::exception& e) {
        // 捕获构造函数抛出的异常（如socket创建失败）
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server stopped" << std::endl;
    return 0;
}
