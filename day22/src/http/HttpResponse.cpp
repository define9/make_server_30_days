#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : m_statusCode(HttpStatusCode::OK), m_chunked(false) {
}

void HttpResponse::reset() {
    m_statusCode = HttpStatusCode::OK;
    m_headers.clear();
    m_body.clear();
}

void HttpResponse::setStatus(HttpStatusCode code) {
    m_statusCode = code;
}

std::string HttpResponse::statusText() const {
    return getStatusText(m_statusCode);
}

void HttpResponse::setBody(const std::string& body) {
    m_body = body;
    setContentLength(body.size());
}

void HttpResponse::setContentType(const std::string& contentType) {
    setHeader("Content-Type", contentType);
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    m_headers[key] = value;
}

void HttpResponse::setKeepAlive(bool keepAlive) {
    if (keepAlive) {
        setHeader("Connection", "keep-alive");
    } else {
        setHeader("Connection", "close");
    }
}

void HttpResponse::setContentLength(size_t length) {
    setHeader("Content-Length", std::to_string(length));
}

void HttpResponse::setChunked(bool chunked) {
    m_chunked = chunked;
    if (chunked) {
        setHeader("Transfer-Encoding", "chunked");
        m_headers.erase("Content-Length");
    } else {
        m_headers.erase("Transfer-Encoding");
    }
}

std::string HttpResponse::build() const {
    std::string result = "HTTP/1.1 " + std::to_string(static_cast<int>(m_statusCode)) + " " + statusText() + "\r\n";

    for (const auto& header : m_headers) {
        result += header.first + ": " + header.second + "\r\n";
    }

    result += "\r\n";
    if (!m_chunked) {
        result += m_body;
    }

    return result;
}

std::string HttpResponse::buildFirstChunk() const {
    std::string result = "HTTP/1.1 " + std::to_string(static_cast<int>(m_statusCode)) + " " + statusText() + "\r\n";

    for (const auto& header : m_headers) {
        result += header.first + ": " + header.second + "\r\n";
    }

    result += "\r\n";
    return result;
}

std::string HttpResponse::buildChunk(const std::string& data) const {
    std::ostringstream oss;
    oss << std::hex << data.size();
    return oss.str() + "\r\n" + data + "\r\n";
}

std::string HttpResponse::buildLastChunk() const {
    return "0\r\n\r\n";
}

std::string HttpResponse::getStatusText(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::OK: return "OK";
        case HttpStatusCode::CREATED: return "Created";
        case HttpStatusCode::NO_CONTENT: return "No Content";
        case HttpStatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatusCode::FOUND: return "Found";
        case HttpStatusCode::NOT_MODIFIED: return "Not Modified";
        case HttpStatusCode::BAD_REQUEST: return "Bad Request";
        case HttpStatusCode::UNAUTHORIZED: return "Unauthorized";
        case HttpStatusCode::FORBIDDEN: return "Forbidden";
        case HttpStatusCode::NOT_FOUND: return "Not Found";
        case HttpStatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatusCode::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatusCode::SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}