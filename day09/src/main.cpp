/**
 * @file main.cpp
 * @brief 程序入口 - Day09 多线程Reactor版本
 * 
 * ==================== 程序流程 ====================
 * 
 * 1. 解析命令行参数（端口号、工作线程数）
 * 2. 创建ReactorPool（管理主Reactor和Worker Reactor池）
 * 3. 注册信号处理器（优雅退出）
 * 4. ReactorPool.start()启动所有Reactor
 * 5. 收到信号后优雅退出
 * 
 * ==================== Day09程序结构 ====================
 * 
 * main.cpp负责：
 * - 程序入口和参数解析
 * - ReactorPool的创建和生命周期管理
 * - 信号处理（优雅退出）
 * 
 * 核心逻辑：
 * - 主Reactor负责accept新连接
 * - Worker Reactor负责处理已建立的连接
 * - 轮询负载均衡分发连接
 */

#include "ReactorPool.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

/**
 * @brief 全局ReactorPool指针
 * 
 * 为什么需要全局变量？
 * - 信号处理函数不能传递参数
 * - 需要在信号处理函数中调用pool.stop()
 */
static ReactorPool* g_pool = nullptr;

/**
 * @brief 信号处理器
 * 
 * 捕获SIGINT和SIGTERM，实现优雅退出。
 */
void signalHandler(int)
{
    if (g_pool) {
        g_pool->stop();
    }
}

/**
 * @brief 打印用法说明
 */
void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " <port> [worker_threads]" << std::endl;
    std::cout << "Example: " << programName << " 8080 4" << std::endl;
    std::cout << "Default: 4 worker threads" << std::endl;
}

int main(int argc, char* argv[])
{
    // ===== 参数解析 =====
    if (argc < 2 || argc > 3) {
        printUsage(argv[0]);
        return 1;
    }

    // 解析端口号
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    if (port == 0) {
        std::cerr << "Error: Invalid port" << std::endl;
        return 1;
    }

    // 解析工作线程数（默认为4）
    int numWorkers = 4;
    if (argc == 3) {
        numWorkers = atoi(argv[2]);
        if (numWorkers <= 0 || numWorkers > 32) {
            std::cerr << "Error: Invalid worker count (1-32)" << std::endl;
            return 1;
        }
    }

    try {
        // ===== 创建ReactorPool =====
        ReactorPool pool(port, numWorkers);

        // ===== 注册信号处理器 =====
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // 设置全局指针（供信号处理函数使用）
        g_pool = &pool;

        // ===== 启动所有Reactor =====
        pool.start();

        // ===== 等待所有线程结束 =====
        pool.waitForExit();

        std::cout << "========== Server Statistics ==========" << std::endl;
        std::cout << "Total connections handled: " << pool.totalConnections() << std::endl;
        std::cout << "======================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server stopped" << std::endl;
    return 0;
}