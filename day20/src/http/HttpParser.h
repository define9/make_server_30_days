#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "HttpRequest.h"
#include "log/Logger.h"

enum class HttpParserState {
    READ_REQUEST_LINE,
    READ_HEADERS,
    READ_BODY,
    CHUNK_SIZE,
    CHUNK_DATA,
    CHUNK_END,
    PARSE_COMPLETE,
    PARSE_ERROR
};

class HttpParser {
public:
    HttpParser();

    void reset();

    HttpParserState state() const { return m_state; }
    const HttpRequest& request() const { return m_request; }
    bool hasCompleteRequest() const { return m_request.isComplete(); }

    size_t parse(const char* data, size_t len);

private:
    size_t parseRequestLine(const char* data, size_t len);
    size_t parseHeaders(const char* data, size_t len);
    size_t parseBody(const char* data, size_t len);

    const char* findCRLF(const char* start, size_t len);
    const char* findHeaderEnd(const char* start, size_t len);
    const char* findCRLFOnly(const char* start, size_t len);

    size_t parseChunkSize(const char* data, size_t len);
    size_t parseChunkData(const char* data, size_t len);
    size_t parseChunkEnd(const char* data, size_t len);

    HttpParserState m_state;
    HttpRequest m_request;
    size_t m_bodyRead;
    bool m_chunkedMode;
    size_t m_chunkSize;
    size_t m_chunkDataRead;
};

#endif