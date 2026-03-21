#include "config/Config.h"
#include "reactor/ReactorPool.h"
#include "thread/ThreadPool.h"
#include "timer/TimerManager.h"
#include "log/Logger.h"
#include "log/Sink.h"
#include <iostream>
#include <csignal>
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

int main(int argc, char* argv[])
{
    Config& config = Config::instance();

    if (!config.loadFromArgs(argc, argv)) {
        Config::instance().printUsage(argv[0]);
        return 1;
    }

    if (!config.validate()) {
        return 1;
    }

    try {
        Logger& logger = Logger::instance();

        if (config.consoleLog()) {
            logger.addSink(std::make_shared<ConsoleSink>());
        }

        if (!config.logFile().empty()) {
            logger.addSink(std::make_shared<FileSink>(config.logFile()));
        }

        if (config.logLevel() == "TRACE") {
            logger.setLevel(Logger::TRACE);
        } else if (config.logLevel() == "DEBUG") {
            logger.setLevel(Logger::DEBUG);
        } else if (config.logLevel() == "WARN") {
            logger.setLevel(Logger::WARN);
        } else if (config.logLevel() == "ERROR") {
            logger.setLevel(Logger::ERROR);
        } else if (config.logLevel() == "FATAL") {
            logger.setLevel(Logger::FATAL);
        } else {
            logger.setLevel(Logger::INFO);
        }

        LOG_INFO("========================================");
        LOG_INFO("  Day14: Configuration System");
        LOG_INFO("========================================");
        LOG_INFO("Config file: %s", config.configFile().empty() ? "(none)" : config.configFile().c_str());
        LOG_INFO("Port: %d", config.port());
        LOG_INFO("Reactor workers: %d", config.reactorWorkers());
        LOG_INFO("Task workers: %d", config.taskWorkers());
        LOG_INFO("Connection timeout: %d seconds", config.connectionTimeout());
        LOG_INFO("Task queue size: %d", config.taskQueueSize());
        LOG_INFO("Log level: %s", config.logLevel().c_str());
        LOG_INFO("Log file: %s", config.logFile().c_str());
        LOG_INFO("Console log: %s", config.consoleLog() ? "enabled" : "disabled");

        ThreadPool taskPool(config.taskWorkers(), config.taskQueueSize());
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

        ReactorPool reactorPool(config.port(), config.reactorWorkers());
        g_pool = &reactorPool;

        LOG_INFO("========================================");
        LOG_INFO("  ReactorPool (Network I/O + Config)");
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