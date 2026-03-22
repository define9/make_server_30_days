#include "config/Config.h"
#include "reactor/ReactorPool.h"
#include "thread/ThreadPool.h"
#include "timer/TimerManager.h"
#include "log/Logger.h"
#include "log/Sink.h"
#include "signal/SignalHandler.h"
#include <iostream>
#include <memory>

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
        LOG_INFO("  Day18: HTTP Long Connection");
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

        ReactorPool reactorPool(config.port(), config.reactorWorkers());

        LOG_INFO("========================================");
        LOG_INFO("  ReactorPool (Network I/O + Signal)");
        LOG_INFO("========================================");

        SignalHandler& signalHandler = SignalHandler::instance();

        signalHandler.onShutdown([&reactorPool]() {
            LOG_INFO("Signal shutdown callback triggered");
            reactorPool.stop();
            LOG_INFO("Signal shutdown callback triggered end");
        });

        signalHandler.onReload([]() {
            LOG_INFO("Configuration reload requested");
        });

        signalHandler.start();
        reactorPool.start();

        LOG_INFO("signalHandler wait done");

        reactorPool.waitForExit();

        LOG_INFO("========================================");
        LOG_INFO("  Final Statistics");
        LOG_INFO("Connections handled by ReactorPool: %ld", (long)reactorPool.totalConnections());

    } catch (const std::exception& e) {
        LOG_ERROR("Exception: %s", e.what());
        return 1;
    }

    LOG_INFO("Server stopped gracefully");
    return 0;
}