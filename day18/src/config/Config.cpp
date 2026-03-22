#include "Config.h"
#include "ConfigFile.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

Config& Config::instance() {
    static Config instance_;
    return instance_;
}

bool Config::loadFromFile(const std::string& filepath) {
    ConfigFile cf;
    if (!cf.load(filepath)) {
        return false;
    }

    configFile_ = filepath;
    applyConfigFile(cf);
    return true;
}

bool Config::loadFromArgs(int argc, char* argv[]) {
    applyDefaults();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                std::string configPath = argv[++i];
                ConfigFile cf;
                if (cf.load(configPath)) {
                    configFile_ = configPath;
                    applyConfigFile(cf);
                }
            }
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "--version") {
            std::cout << "Server version 1.0.0 (Day14 - Configuration System)\n";
            exit(0);
        }
    }

    return true;
}

void Config::applyDefaults() {
    port_ = 8080;
    reactorWorkers_ = 4;
    taskWorkers_ = 4;
    connectionTimeout_ = 30;
    taskQueueSize_ = 100;
    logLevel_ = "DEBUG";
    logFile_ = "server.log";
    consoleLog_ = true;
}

void Config::applyConfigFile(const ConfigFile& cf) {
    if (cf.hasSection("server")) {
        if (cf.hasKey("server", "port")) {
            port_ = static_cast<uint16_t>(cf.getInt("server", "port", port_));
        }
        if (cf.hasKey("server", "reactor_workers")) {
            reactorWorkers_ = cf.getInt("server", "reactor_workers", reactorWorkers_);
        }
        if (cf.hasKey("server", "task_workers")) {
            taskWorkers_ = cf.getInt("server", "task_workers", taskWorkers_);
        }
        if (cf.hasKey("server", "connection_timeout")) {
            connectionTimeout_ = cf.getInt("server", "connection_timeout", connectionTimeout_);
        }
        if (cf.hasKey("server", "task_queue_size")) {
            taskQueueSize_ = cf.getInt("server", "task_queue_size", taskQueueSize_);
        }
    }

    if (cf.hasSection("log")) {
        if (cf.hasKey("log", "level")) {
            logLevel_ = cf.get("log", "level", logLevel_);
        }
        if (cf.hasKey("log", "file")) {
            logFile_ = cf.get("log", "file", logFile_);
        }
        if (cf.hasKey("log", "console")) {
            consoleLog_ = cf.getBool("log", "console", consoleLog_);
        }
    }
}

void Config::printUsage(const char* programName) const {
    std::cout << "Usage: " << programName << " [OPTIONS]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -c, --config <file>   Configuration file (default: server.conf)\n";
    std::cout << "  -h, --help            Show this help message\n";
    std::cout << "      --version         Show version info\n";
    std::cout << "\nConfiguration file example (server.conf):\n";
    std::cout << "  [server]\n";
    std::cout << "  port = 8080\n";
    std::cout << "  reactor_workers = 4\n";
    std::cout << "  task_workers = 4\n";
    std::cout << "  connection_timeout = 30\n";
    std::cout << "  task_queue_size = 100\n";
    std::cout << "\n  [log]\n";
    std::cout << "  level = INFO\n";
    std::cout << "  file = server.log\n";
    std::cout << "  console = true\n";
}

bool Config::validate() const {
    if (port_ == 0) {
        std::cerr << "Invalid port: " << port_ << "\n";
        return false;
    }
    if (reactorWorkers_ <= 0 || reactorWorkers_ > 32) {
        std::cerr << "Invalid reactor_workers: " << reactorWorkers_ << " (must be 1-32)\n";
        return false;
    }
    if (taskWorkers_ <= 0 || taskWorkers_ > 32) {
        std::cerr << "Invalid task_workers: " << taskWorkers_ << " (must be 1-32)\n";
        return false;
    }
    if (connectionTimeout_ <= 0) {
        std::cerr << "Invalid connection_timeout: " << connectionTimeout_ << "\n";
        return false;
    }
    return true;
}