/**
 * HTTP 请求解析器 - 增量式状态机
 *
 * 设计原则: parse() 返回本次消费的字节数，调用方负责裁剪 buffer。
 *
 * 字节偏移量约定:
 *   - findCRLF() 返回指向 '\r' 的指针
 *   - 所以 +2 才能跳过完整的 CRLF (跳到下一行开头)
 *   - findHeaderEnd() 返回指向 "\r\n\r\n" 第一个 '\r' 的指针
 *   - 所以 +4 才能跳过完整的 header 结束标记
 *
 * 示例:
 *   data = "GET / HTTP/1.1\r\n\r\n"
 *                ↑           ↑
 *              crlf      headerEnd (指向第2个\r)
 *   parseRequestLine 返回: (crlf-data) + 2 = 12 + 2 = 14
 *   parseHeaders 返回: (headerEnd-data) + 4 = 14 + 4 = 18
 */
#include "HttpParser.h"
#include <cstring>
#include <sstream>

HttpParser::HttpParser() : m_state(HttpParserState::READ_REQUEST_LINE), m_bodyRead(0),
    m_chunkedMode(false), m_chunkSize(0), m_chunkDataRead(0) {
}

void HttpParser::reset() {
    m_state = HttpParserState::READ_REQUEST_LINE;
    m_request.reset();
    m_bodyRead = 0;
    m_chunkedMode = false;
    m_chunkSize = 0;
    m_chunkDataRead = 0;
}

size_t HttpParser::parse(const char* data, size_t len) {
    size_t consumed = 0;

    // 这里传 consumed <= len 带等于，因为可能没有body，也让他进body状态直接返回叭
    while (consumed <= len && m_state != HttpParserState::PARSE_COMPLETE && m_state != HttpParserState::PARSE_ERROR) {
        size_t needed = 0;

        switch (m_state) {
            case HttpParserState::READ_REQUEST_LINE:
                needed = parseRequestLine(data + consumed, len - consumed);
                break;
            case HttpParserState::READ_HEADERS:
                needed = parseHeaders(data + consumed, len - consumed);
                break;
            case HttpParserState::READ_BODY:
                needed = parseBody(data + consumed, len - consumed);
                break;
            case HttpParserState::CHUNK_SIZE:
                needed = parseChunkSize(data + consumed, len - consumed);
                break;
            case HttpParserState::CHUNK_DATA:
                needed = parseChunkData(data + consumed, len - consumed);
                break;
            case HttpParserState::CHUNK_END:
                needed = parseChunkEnd(data + consumed, len - consumed);
                break;
            default:
                return consumed;
        }

        consumed += needed;
    }

    return consumed;
}

const char* HttpParser::findCRLF(const char* start, size_t len) {
    for (size_t i = 0; i + 1 < len; ++i) {
        if (start[i] == '\r' && start[i + 1] == '\n') {
            return start + i;
        }
    }
    return nullptr;
}

const char* HttpParser::findHeaderEnd(const char* start, size_t len) {
    for (size_t i = 0; i + 3 < len; ++i) {
        if (start[i] == '\r' && start[i + 1] == '\n' &&
            start[i + 2] == '\r' && start[i + 3] == '\n') {
            return start + i;
        }
    }
    return nullptr;
}

size_t HttpParser::parseRequestLine(const char* data, size_t len) {
    const char* crlf = findCRLF(data, len);
    if (!crlf) return 0;  // 请求行不完整，需要更多数据

    if (!m_request.parseRequestLine(data, crlf)) {
        m_state = HttpParserState::PARSE_ERROR;
        return 0;
    }

    m_state = HttpParserState::READ_HEADERS;
    // 返回消费的字节数: crlf指向\r，位置差+2跳过\r\n
    return (crlf - data) + 2;
}

size_t HttpParser::parseHeaders(const char* data, size_t len) {
    // 先尝试找完整的 header 块 (\r\n\r\n)
    const char* headerEnd = findHeaderEnd(data, len);
    if (!headerEnd) {
        // 没有完整的 header 结束标记，检查是否收到空行 (GET请求直接结束的情况)
        // 场景: 数据以 \r\n\r\n 开头，说明是空行（header 立即结束）
        const char* crlf = findCRLF(data, len);
        // crlf == data 说明第一个字符就是 \r（即以 CRLF 开头）
        if (crlf && crlf == data && m_request.headers().empty()) {
            // data = "\r\n..."
            //      ↑
            //     crlf (position 0)
            // 检查下一个 CRLF 是否紧跟在后面 (data + 2 跳过第一个 \r\n)
            const char* nextCrlf = findCRLF(data + 2, len - 2);
            // nextCrlf == data + 2 说明是连续的 \r\n\r\n
            if (nextCrlf && nextCrlf == data + 2) {
                // POST 等方法，需要读 body
                m_state = HttpParserState::READ_BODY;
                // 跳过这个空行，进入 READ_BODY 状态
                return (crlf - data) + 2;  // = 0 + 2 = 2，消费 \r\n
            }
        }
        return 0;  // 数据不完整，等待更多数据
    }

    // 找到完整的 header 块，逐行解析
    const char* pos = data;
    while (pos < headerEnd) {
        // 在 [pos, headerEnd) 范围内找行尾 CRLF
        const char* lineEnd = findCRLF(pos, headerEnd - pos);
        if (!lineEnd) break;  // 行不完整

        size_t lineLen = lineEnd - pos;  // 不包含 \r\n 的行长度
        if (lineLen > 0) {
            // 找冒号分隔 key:value
            const char* colon = static_cast<const char*>(memchr(pos, ':', lineLen));
            if (colon) {
                // key: [pos, colon)
                std::string key(pos, colon - pos);
                // value: [colon+1, lineEnd) - 冒号后到行尾
                std::string value(colon + 1, lineEnd - colon - 1);

                // 去除 value 首尾的空白字符 (RFC 规定)
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                    value = value.substr(1);
                }
                while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                    value.pop_back();
                }

                m_request.addHeader(key, value);
            }
        }

        // 移动到下一行: lineEnd 指向 \r，所以 +2 跳过 \r\n
        pos = lineEnd + 2;
    }

    // 有 body，进入 READ_BODY 状态
    // headerEnd 指向 \r\n\r\n 的第一个 \r
    // (headerEnd - data) + 4 跳过整个 header 区域
    //   - headerEnd - data: 到第一个 \r 的距离
    //   - +4: 跳过 \r\n\r\n (4个字节)
    m_state = HttpParserState::READ_BODY;
    return (headerEnd - data) + 4;
}

size_t HttpParser::parseBody(const char* data, size_t len) {
    int64_t contentLength = m_request.contentLength();

    if (m_request.hasHeader("Transfer-Encoding")) {
        std::string te = m_request.getHeader("Transfer-Encoding");
        if (te == "chunked") {
            m_chunkedMode = true;
            m_state = HttpParserState::CHUNK_SIZE;
            return 0;
        }
    }

    if (contentLength == 0) {
        m_request.setComplete(true);
        m_state = HttpParserState::PARSE_COMPLETE;
        return 0;
    }

    size_t bodyLen = len;
    if (m_bodyRead + bodyLen > static_cast<size_t>(contentLength)) {
        bodyLen = contentLength - m_bodyRead;
    }

    m_bodyRead += bodyLen;

    if (m_bodyRead >= static_cast<size_t>(contentLength)) {
        m_request.setComplete(true);
        m_state = HttpParserState::PARSE_COMPLETE;
    }

    return bodyLen;
}

const char* HttpParser::findCRLFOnly(const char* start, size_t len) {
    for (size_t i = 0; i + 1 < len; ++i) {
        if (start[i] == '\r' && start[i + 1] == '\n') {
            return start + i;
        }
    }
    return nullptr;
}

size_t HttpParser::parseChunkSize(const char* data, size_t len) {
    const char* crlf = findCRLFOnly(data, len);
    if (!crlf) return 0;

    size_t lineLen = crlf - data;
    if (lineLen == 0) {
        m_state = HttpParserState::PARSE_ERROR;
        return 0;
    }

    std::string sizeStr(data, lineLen);
    std::istringstream iss(sizeStr);
    iss >> std::hex >> m_chunkSize;
    m_chunkDataRead = 0;

    if (m_chunkSize == 0) {
        m_request.setComplete(true);
        m_state = HttpParserState::PARSE_COMPLETE;
    } else {
        m_state = HttpParserState::CHUNK_DATA;
    }

    return (crlf - data) + 2;
}

size_t HttpParser::parseChunkData(const char* data, size_t len) {
    size_t remaining = m_chunkSize - m_chunkDataRead;
    size_t toConsume = std::min(remaining, len);

    m_chunkDataRead += toConsume;

    if (m_chunkDataRead >= m_chunkSize) {
        m_state = HttpParserState::CHUNK_END;
    }

    return toConsume;
}

size_t HttpParser::parseChunkEnd(const char* data, size_t len) {
    if (len < 2) return 0;

    if (data[0] == '\r' && data[1] == '\n') {
        m_state = HttpParserState::CHUNK_SIZE;
        return 2;
    }

    m_state = HttpParserState::PARSE_ERROR;
    return 0;
}