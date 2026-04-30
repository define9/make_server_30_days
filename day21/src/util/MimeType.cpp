#include "MimeType.h"
#include <algorithm>
#include <cstring>

std::map<std::string, std::string> MimeType::buildMimeMap() {
    std::map<std::string, std::string> m;
    m[".html"] = "text/html";
    m[".htm"] = "text/html";
    m[".css"] = "text/css";
    m[".js"] = "application/javascript";
    m[".json"] = "application/json";
    m[".xml"] = "application/xml";
    m[".txt"] = "text/plain";
    m[".png"] = "image/png";
    m[".jpg"] = "image/jpeg";
    m[".jpeg"] = "image/jpeg";
    m[".gif"] = "image/gif";
    m[".svg"] = "image/svg+xml";
    m[".ico"] = "image/x-icon";
    m[".pdf"] = "application/pdf";
    m[".zip"] = "application/zip";
    m[".gz"] = "application/gzip";
    m[".tar"] = "application/x-tar";
    m[".mp3"] = "audio/mpeg";
    m[".mp4"] = "video/mp4";
    m[".webm"] = "video/webm";
    m[".woff"] = "font/woff";
    m[".woff2"] = "font/woff2";
    m[".ttf"] = "font/ttf";
    m[".eot"] = "application/vnd.ms-fontobject";
    return m;
}

const std::map<std::string, std::string>& MimeType::mimeMap() {
    static std::map<std::string, std::string> s_mimeMap = buildMimeMap();
    return s_mimeMap;
}

std::string MimeType::getExtension(const std::string& path) {
    size_t pos = path.rfind('.');
    if (pos == std::string::npos) {
        return "";
    }
    std::string ext = path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

const std::string& MimeType::get(const std::string& path) {
    static const std::string s_defaultType = "application/octet-stream";
    const auto& m = mimeMap();
    std::string ext = getExtension(path);
    if (ext.empty() || m.find(ext) == m.end()) {
        return s_defaultType;
    }
    return m.at(ext);
}