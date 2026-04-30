#include "Formatter.h"
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

Formatter::Formatter(const std::string& pattern)
    : m_pattern(pattern)
{
}

std::string Formatter::format(int level, const char* file, int line, const char* msg)
{
    char buffer[8192];
    size_t pos = 0;
    size_t len = m_pattern.size();
    
    for (size_t i = 0; i < len && pos < sizeof(buffer) - 1; ++i) {
        if (m_pattern[i] != '%') {
            buffer[pos++] = m_pattern[i];
            continue;
        }
        
        if (i + 1 >= len) break;
        
        switch (m_pattern[++i]) {
            case '%':
                buffer[pos++] = '%';
                break;
            case 'd': {
                time_t now = time(nullptr);
                pos += strftime(buffer + pos, sizeof(buffer) - pos, "%Y-%m-%d", localtime(&now));
                break;
            }
            case 'T': {
                struct timeval tv;
                gettimeofday(&tv, nullptr);
                struct tm* tm_info = localtime(&tv.tv_sec);
                pos += strftime(buffer + pos, sizeof(buffer) - pos, "%H:%M:%S", tm_info);
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, ".%03ld", tv.tv_usec / 1000);
                break;
            }
            case 'l':
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%s", levelToString(level).c_str());
                break;
            case 't':
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%lu", (unsigned long)pthread_self());
                break;
            case 'f':
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%s", file);
                break;
            case 'L':
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%d", line);
                break;
            case 'm':
                pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%s", msg);
                break;
            default:
                buffer[pos++] = '%';
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = m_pattern[i];
                }
                break;
        }
    }
    
    buffer[pos++] = '\n';
    buffer[pos] = '\0';
    
    return std::string(buffer);
}

std::string Formatter::levelToString(int level)
{
    switch (level) {
        case 0: return "TRACE";
        case 1: return "DEBUG";
        case 2: return "INFO";
        case 3: return "WARN";
        case 4: return "ERROR";
        case 5: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Formatter::now()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm* tm_info = localtime(&tv.tv_sec);
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char result[128];
    snprintf(result, sizeof(result), "%s.%03ld", buffer, tv.tv_usec / 1000);
    
    return std::string(result);
}
