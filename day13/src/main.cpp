#include "reactor/ReactorPool.h"
#include "thread/ThreadPool.h"
#include "timer/TimerManager.h"
#include "log/Logger.h"
#include "log/Sink.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>

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
    std::cout << "\nConnection timeout: 30 seconds (idle connections are closed)" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 4) {
        printUsage(argv[0]);
        return 1;
    }

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    if (port == 0) {
        LOG_ERROR("Invalid port");
        return 1;
    }

    int numReactorWorkers = 4;
    int numTaskWorkers = 4;
    
    if (argc >= 3) {
        numReactorWorkers = atoi(argv[2]);
        if (numReactorWorkers <= 0 || numReactorWorkers > 32) {
            LOG_ERROR("Invalid reactor worker count (1-32)");
            return 1;
        }
    }
    
    if (argc >= 4) {
        numTaskWorkers = atoi(argv[3]);
        if (numTaskWorkers <= 0 || numTaskWorkers > 32) {
            LOG_ERROR("Invalid task worker count (1-32)");
            return 1;
        }
    }

    try {
        Logger& logger = Logger::instance();
        logger.addSink(std::make_shared<ConsoleSink>());
        logger.addSink(std::make_shared<FileSink>("server.log"));
        logger.setLevel(Logger::INFO);

        LOG_INFO("========================================");
        LOG_INFO("  Day13: Logging System");
        LOG_INFO("========================================");
        
        LOG_INFO("Connection timeout demo (30 seconds idle)");
        LOG_INFO("Connect with: nc localhost %d", port);
        LOG_INFO("Then wait 30 seconds without sending data");
        
        ThreadPool taskPool(numTaskWorkers, 100);
        g_threadPool = &taskPool;
        
        LOG_INFO("========================================");
        LOG_INFO("  ThreadPool Demo");
        LOG_INFO("========================================");
        
        taskPool.start();
        
        LOG_INFO("Submitting demo tasks to ThreadPool...");
        for (int i = 0; i < 3; ++i) {
            taskPool.submit([i]() {
                LOG_INFO("Worker %d executing...", i);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                g_tasksExecuted.fetch_add(1);
            });
        }
        
        taskPool.waitForIdle();
        LOG_INFO("Demo tasks completed: %ld executed", (long)g_tasksExecuted.load());
        
        ReactorPool reactorPool(port, numReactorWorkers);
        g_pool = &reactorPool;

        LOG_INFO("========================================");
        LOG_INFO("  ReactorPool (Network I/O + Timeout)");
        LOG_INFO("========================================");
        
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        reactorPool.start();

        reactorPool.waitForExit();

        LOG_INFO("========================================");
        LOG_INFO("  Final Statistics");
        LOG_INFO("========================================");
        LOG_INFO("Tasks executed by ThreadPool: %ld", (long)g_tasksExecuted.load());
        LOG_INFO("Connections handled by ReactorPool: %ld", (long)reactorPool.totalConnections());

    } catch (const std::exception& e) {
        LOG_ERROR("Exception: %s", e.what());
        return 1;
    }

    LOG_INFO("Server stopped");
    return 0;
}
