#include "FileUtil.h"
#include <fstream>
#include <sys/stat.h>

bool FileUtil::readFile(const std::string& path, std::string& content) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    if (size < 0) {
        return false;
    }

    file.seekg(0, std::ios::beg);
    content.resize(size);
    file.read(&content[0], size);
    return file.good() || file.eof();
}

std::string FileUtil::getExt(const std::string& path) {
    size_t pos = path.rfind('.');
    if (pos == std::string::npos) {
        return "";
    }
    return path.substr(pos);
}

bool FileUtil::exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}