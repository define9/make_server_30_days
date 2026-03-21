/**
 * @file main.cpp
 * @brief Day11 定时器实现 - 最小堆与时间轮
 */

#include "reactor/ReactorPool.h"
#include "thread/ThreadPool.h"
#include "timer/TimerManager.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <chrono>

static ReactorPool* g_pool = nullptr;
static ThreadPool* g_threadPool = nullptr;
static std::atomic<uint64_t> g_tasksExecuted{0};
static std::atomic<int64_t> g_tickCount{0};

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
        std::cout << "========================================" << std::endl;
        std::cout << "  Day11: Timer Implementation Demo" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // Timer demo using TimerHeap
        std::cout << "\n[Timer] Testing TimerHeap (Minimum Heap)..." << std::endl;
        {
            TimerManager heapTimer(TimerType::HEAP, 100);
            
            int64_t timer1 = heapTimer.addTimer(500, 0, [](int64_t id) {
                std::cout << "[Timer] One-shot timer " << id << " expired!" << std::endl;
            });
            
            int64_t timer2 = heapTimer.addTimer(200, 0, [](int64_t id) {
                std::cout << "[Timer] Timer " << id << " expired!" << std::endl;
            });
            
            int64_t timer3 = heapTimer.addTimer(300, 100, [](int64_t id) {
                std::cout << "[Timer] Repeating timer " << id << " fired!" << std::endl;
            });
            
            std::cout << "[Timer] Added 3 timers (id=" << timer1 << "," << timer2 << "," << timer3 << "), waiting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            
            for (int i = 0; i < 10; ++i) {
                int64_t wait = heapTimer.tick();
                if (wait > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(wait));
                }
            }
        }
        
        // Timer demo using TimerWheel
        std::cout << "\n[Timer] Testing TimerWheel..." << std::endl;
        {
            TimerManager wheelTimer(TimerType::WHEEL, 100);
            
            int64_t timer1 = wheelTimer.addTimer(250, 0, [](int64_t id) {
                std::cout << "[Timer] Wheel timer " << id << " expired!" << std::endl;
            });
            
            int64_t timer2 = wheelTimer.addTimer(500, 0, [](int64_t id) {
                std::cout << "[Timer] Wheel timer " << id << " expired!" << std::endl;
            });
            
            std::cout << "[Timer] Added 2 wheel timers (id=" << timer1 << "," << timer2 << "), waiting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            
            for (int i = 0; i < 10; ++i) {
                int64_t wait = wheelTimer.tick();
                if (wait > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(wait));
                }
            }
        }
        
        // ThreadPool demo
        ThreadPool taskPool(numTaskWorkers, 100);
        g_threadPool = &taskPool;
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  ThreadPool Demo" << std::endl;
        std::cout << "========================================" << std::endl;
        
        taskPool.start();
        
        std::cout << "\n[Main] Submitting demo tasks to ThreadPool..." << std::endl;
        for (int i = 0; i < 3; ++i) {
            taskPool.submit([i]() {
                std::cout << "[Task] Worker " << i << " executing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                g_tasksExecuted.fetch_add(1);
            });
        }
        
        taskPool.waitForIdle();
        std::cout << "[Main] Demo tasks completed: " << g_tasksExecuted.load() << " executed" << std::endl;
        
        // ReactorPool with timer support
        ReactorPool reactorPool(port, numReactorWorkers);
        g_pool = &reactorPool;

        std::cout << "\n========================================" << std::endl;
        std::cout << "  ReactorPool (Network I/O + Timer)" << std::endl;
        std::cout << "========================================" << std::endl;
        
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        reactorPool.start();

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