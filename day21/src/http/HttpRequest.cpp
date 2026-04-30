#include "HttpRequest.h"
#include <algorithm>
#include <cstring>

HttpRequest::HttpRequest() : m_method(HttpMethod::INVALID), m_version(HttpVersion::INVALID), m_complete(false) {
}

void HttpRequest::reset() {
    m_method = HttpMethod::INVALID;
    m_version = HttpVersion::INVALID;
    m_url.clear();
    m_path.clear();
    m_headers.clear();
    m_complete = false;
}

bool HttpRequest::parseRequestLine(const char* start, const char* end) {
    std::string line(start, end);

    size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos) return false;

    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) return false;

    std::string methodStr = line.substr(0, firstSpace);
    std::transform(methodStr.begin(), methodStr.end(), methodStr.begin(), ::toupper);

    if (methodStr == "GET") m_method = HttpMethod::GET;
    else if (methodStr == "POST") m_method = HttpMethod::POST;
    else if (methodStr == "PUT") m_method = HttpMethod::PUT;
    else if (methodStr == "DELETE") m_method = HttpMethod::DELETE;
    else if (methodStr == "HEAD") m_method = HttpMethod::HEAD;
    else if (methodStr == "OPTIONS") m_method = HttpMethod::OPTIONS;
    else if (methodStr == "PATCH") m_method = HttpMethod::PATCH;
    else m_method = HttpMethod::INVALID;

    m_url = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    size_t queryPos = m_url.find('?');
    if (queryPos != std::string::npos) {
        m_path = m_url.substr(0, queryPos);
    } else {
        m_path = m_url;
    }

    std::string versionStr = line.substr(secondSpace + 1);
    std::transform(versionStr.begin(), versionStr.end(), versionStr.begin(), ::toupper);
    if (versionStr == "HTTP/1.1") m_version = HttpVersion::HTTP_1_1;
    else if (versionStr == "HTTP/1.0") m_version = HttpVersion::HTTP_1_0;
    else m_version = HttpVersion::INVALID;

    return m_method != HttpMethod::INVALID && m_version != HttpVersion::INVALID;
}

void HttpRequest::addHeader(const std::string& key, const std::string& value) {
    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    m_headers[lowerKey] = value;
}

std::string HttpRequest::methodString() const {
    switch (m_method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::PATCH: return "PATCH";
        default: return "INVALID";
    }
}

std::string HttpRequest::path() const {
    return m_path.empty() ? "/" : m_path;
}

std::string HttpRequest::query() const {
    size_t queryPos = m_url.find('?');
    if (queryPos != std::string::npos && queryPos + 1 < m_url.size()) {
        return m_url.substr(queryPos + 1);
    }
    return "";
}

const std::string& HttpRequest::getHeader(const std::string& key) const {
    static const std::string empty;
    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    auto it = m_headers.find(lowerKey);
    if (it != m_headers.end()) {
        return it->second;
    }
    return empty;
}

bool HttpRequest::hasHeader(const std::string& key) const {
    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
    return m_headers.find(lowerKey) != m_headers.end();
}

bool HttpRequest::keepAlive() const {
    if (m_version != HttpVersion::HTTP_1_1) return false;
    auto it = m_headers.find("connection");
    if (it != m_headers.end()) {
        std::string val = it->second;
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        return val != "close";
    }
    return true;
}

int64_t HttpRequest::contentLength() const {
    auto it = m_headers.find("content-length");
    if (it != m_headers.end()) {
        try {
            return std::stoll(it->second);
        } catch (...) {
            return 0;
        }
    }
    return 0;
}