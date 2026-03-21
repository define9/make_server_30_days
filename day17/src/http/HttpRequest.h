/**
 * @file HttpRequest.h
 * @brief HTTP请求 - 请求方法、URL、头部解析
 */

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <cstdint>

enum class HttpMethod {
    INVALID,
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH
};

enum class HttpVersion {
    HTTP_1_0,
    HTTP_1_1,
    INVALID
};

class HttpRequest {
public:
    HttpRequest();

    // 重置状态
    void reset();

    // 解析请求行
    bool parseRequestLine(const char* start, const char* end);

    // 添加头部
    void addHeader(const std::string& key, const std::string& value);

    // 获取方法
    HttpMethod method() const { return m_method; }
    std::string methodString() const;

    // 获取URL
    const std::string& url() const { return m_url; }
    std::string path() const;
    std::string query() const;

    // 获取版本
    HttpVersion version() const { return m_version; }

    // 获取头部
    const std::string& getHeader(const std::string& key) const;
    bool hasHeader(const std::string& key) const;
    const std::map<std::string, std::string>& headers() const { return m_headers; }

    // 判断是否为完整请求
    bool isComplete() const { return m_complete; }
    void setComplete(bool complete) { m_complete = complete; }

    // 长连接
    bool keepAlive() const;

    // 内容长度
    int64_t contentLength() const;

private:
    HttpMethod m_method;
    HttpVersion m_version;
    std::string m_url;
    std::string m_path;
    std::map<std::string, std::string> m_headers;
    bool m_complete;
};

#endif // HTTP_REQUEST_H