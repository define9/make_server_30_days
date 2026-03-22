#ifndef LOGGER_H
#define LOGGER_H

#include "Sink.h"
#include "Formatter.h"
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdarg>

class Logger {
public:
    enum Level {
        TRACE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };
    
    static Logger& instance();
    
    explicit Logger(Level level);
    ~Logger();
    
    void setLevel(Level level) { m_level.store(static_cast<int>(level)); }
    Level level() const { return static_cast<Level>(m_level.load()); }
    
    void addSink(std::shared_ptr<Sink> sink) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.push_back(sink);
    }
    
    void log(Level level, const char* file, int line, const char* fmt, va_list args);
    void log(Level level, const char* file, int line, const char* fmt, ...);
    
    void trace(const char* file, int line, const char* fmt, ...);
    void debug(const char* file, int line, const char* fmt, ...);
    void info(const char* file, int line, const char* fmt, ...);
    void warn(const char* file, int line, const char* fmt, ...);
    void error(const char* file, int line, const char* fmt, ...);
    void fatal(const char* file, int line, const char* fmt, ...);

private:
    void write(Level level, const char* file, int line, const char* message);

    std::vector<std::shared_ptr<Sink>> m_sinks;
    Formatter m_formatter;
    std::atomic<int> m_level;
    std::mutex m_mutex;
};

#define LOG_TRACE(fmt, ...) \
    Logger::instance().trace(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) \
    Logger::instance().debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    Logger::instance().info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
    Logger::instance().warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    Logger::instance().error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) \
    Logger::instance().fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif // LOGGER_H
