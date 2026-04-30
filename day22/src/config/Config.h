#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <memory>
#include <cstdint>

class ConfigFile;

class Config {
public:
    static Config& instance();

    bool loadFromFile(const std::string& filepath);
    bool loadFromArgs(int argc, char* argv[]);

    uint16_t port() const { return port_; }
    int reactorWorkers() const { return reactorWorkers_; }
    int taskWorkers() const { return taskWorkers_; }
    int connectionTimeout() const { return connectionTimeout_; }
    int taskQueueSize() const { return taskQueueSize_; }

    std::string logLevel() const { return logLevel_; }
    std::string logFile() const { return logFile_; }
    bool consoleLog() const { return consoleLog_; }

    std::string configFile() const { return configFile_; }

    void printUsage(const char* programName) const;
    bool validate() const;

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void applyDefaults();
    void applyConfigFile(const ConfigFile& cf);
    bool parseArgs(int argc, char* argv[]);

    std::string configFile_;

    uint16_t port_ = 8080;
    int reactorWorkers_ = 4;
    int taskWorkers_ = 4;
    int connectionTimeout_ = 30;
    int taskQueueSize_ = 100;

    std::string logLevel_ = "INFO";
    std::string logFile_ = "server.log";
    bool consoleLog_ = true;
};

#endif // CONFIG_H