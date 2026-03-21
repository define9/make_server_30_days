#include "Logger.h"
#include <cstdio>

Logger& Logger::instance()
{
    static Logger logger(INFO);
    return logger;
}

Logger::Logger(Level level)
    : m_formatter("[%d %T] [%l] [%f:%L] %m")
    , m_level(static_cast<int>(level))
{
}

Logger::~Logger()
{
}

void Logger::log(Level level, const char* file, int line, const char* fmt, va_list args)
{
    if (static_cast<int>(level) < m_level.load()) {
        return;
    }
    
    char buffer[8192];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    write(level, file, line, buffer);
}

void Logger::log(Level level, const char* file, int line, const char* fmt, ...)
{
    if (static_cast<int>(level) < m_level.load()) {
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    log(level, file, line, fmt, args);
    va_end(args);
}

void Logger::write(Level level, const char* file, int line, const char* message)
{
    std::string formatted = m_formatter.format(level, file, line, message);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sink : m_sinks) {
        sink->write(formatted);
    }
}

void Logger::trace(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log(TRACE, file, line, fmt, args);
    va_end(args);
}

void Logger::debug(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log(DEBUG, file, line, fmt, args);
    va_end(args);
}

void Logger::info(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log(INFO, file, line, fmt, args);
    va_end(args);
}

void Logger::warn(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log(WARN, file, line, fmt, args);
    va_end(args);
}

void Logger::error(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log(ERROR, file, line, fmt, args);
    va_end(args);
}

void Logger::fatal(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log(FATAL, file, line, fmt, args);
    va_end(args);
}
