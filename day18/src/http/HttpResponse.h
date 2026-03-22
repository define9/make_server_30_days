#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>

enum class HttpStatusCode {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

class HttpResponse {
public:
    HttpResponse();

    void reset();

    void setStatus(HttpStatusCode code);
    HttpStatusCode status() const { return m_statusCode; }
    std::string statusText() const;

    void setBody(const std::string& body);
    const std::string& body() const { return m_body; }

    void setContentType(const std::string& contentType);
    void setHeader(const std::string& key, const std::string& value);
    void setKeepAlive(bool keepAlive);
    void setContentLength(size_t length);

    std::string build() const;

    static std::string getStatusText(HttpStatusCode code);

private:
    HttpStatusCode m_statusCode;
    std::map<std::string, std::string> m_headers;
    std::string m_body;
};

#endif