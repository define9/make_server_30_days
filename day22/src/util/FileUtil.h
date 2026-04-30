#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <string>

class FileUtil {
public:
    static bool readFile(const std::string& path, std::string& content);
    static std::string getExt(const std::string& path);
    static bool exists(const std::string& path);
};

#endif