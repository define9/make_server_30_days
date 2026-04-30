#ifndef MIME_TYPE_H
#define MIME_TYPE_H

#include <string>
#include <map>

class MimeType {
public:
    static const std::string& get(const std::string& path);

private:
    MimeType() = default;

    static std::map<std::string, std::string> buildMimeMap();
    static std::string getExtension(const std::string& path);

    static const std::map<std::string, std::string>& mimeMap();
};

#endif