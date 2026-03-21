/**
 * @file main.cpp
 * @brief Day10 线程池实现 - 演示独立线程池
 * 
 * Day10的核心变化：
 * 1. 引入独立的ThreadPool类（任务队列 + Worker线程）
 * 2. ReactorPool基于ThreadPool的概念，但用于事件处理
 * 3. 演示ThreadPool的通用任务处理能力
 */

#include "reactor/ReactorPool.h"
#include "thread/ThreadPool.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <chrono>

static ReactorPool* g_pool = nullptr;
static ThreadPool* g_threadPool = nullptr;
static std::atomic<uint64_t> g_tasksExecuted{0};

void signalHandler(int)
{
    if (g_pool) {
        g_pool->stop();
    }
    if (g_threadPool) {
        g_threadPool->stop();
    }
}

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " <port> [worker_threads] [task_threads]" << std::endl;
    std::cout << "Example: " << programName << " 8080 4 4" << std::endl;
    std::cout << "  - Default: 4 reactor workers, 4 task workers" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 4) {
        printUsage(argv[0]);
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    if (port == 0) {
        std::cerr << "Error: Invalid port" << std::endl;
        return 1;
    }

    int numReactorWorkers = 4;
    int numTaskWorkers = 4;
    
    if (argc >= 3) {
        numReactorWorkers = atoi(argv[2]);
        if (numReactorWorkers <= 0 || numReactorWorkers > 32) {
            std::cerr << "Error: Invalid reactor worker count (1-32)" << std::endl;
            return 1;
        }
    }
    
    if (argc >= 4) {
        numTaskWorkers = atoi(argv[3]);
        if (numTaskWorkers <= 0 || numTaskWorkers > 32) {
            std::cerr << "Error: Invalid task worker count (1-32)" << std::endl;
            return 1;
        }
    }

    try {
        // 创建独立的线程池（用于一般任务）
        ThreadPool taskPool(numTaskWorkers, 100);
        g_threadPool = &taskPool;
        
        std::cout << "========================================" << std::endl;
        std::cout << "  Day10: ThreadPool Demo" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // 启动任务线程池
        taskPool.start();
        
        // 演示：提交一些测试任务
        std::cout << "\n[Main] Submitting demo tasks to ThreadPool..." << std::endl;
        for (int i = 0; i < 3; ++i) {
            taskPool.submit([i]() {
                std::cout << "[Task] Worker " << i << " executing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                g_tasksExecuted.fetch_add(1);
            });
        }
        
        // 等待任务完成
        taskPool.waitForIdle();
        std::cout << "[Main] Demo tasks completed: " << g_tasksExecuted.load() << " executed" << std::endl;
        
        // 创建Reactor线程池（用于网络I/O）
        ReactorPool reactorPool(port, numReactorWorkers);
        g_pool = &reactorPool;

        std::cout << "\n========================================" << std::endl;
        std::cout << "  ReactorPool (Network I/O)" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // 注册信号处理器
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // 启动Reactor池
        reactorPool.start();

        // 等待退出
        reactorPool.waitForExit();

        std::cout << "\n========================================" << std::endl;
        std::cout << "  Final Statistics" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Tasks executed by ThreadPool: " << g_tasksExecuted.load() << std::endl;
        std::cout << "Connections handled by ReactorPool: " << reactorPool.totalConnections() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nServer stopped" << std::endl;
    return 0;
}