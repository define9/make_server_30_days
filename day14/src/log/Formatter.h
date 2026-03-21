#ifndef FORMATTER_H
#define FORMATTER_H

#include <string>
#include <ctime>

class Formatter {
public:
    explicit Formatter(const std::string& pattern = "[%d %T] [%l] [%f:%L] %m");
    
    std::string format(int level, const char* file, int line, const char* msg);
    
    static std::string levelToString(int level);
    static std::string now();

private:
    std::string m_pattern;
};

#endif // FORMATTER_H
