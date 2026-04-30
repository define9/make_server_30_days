#include "SimpleRouter.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "log/Logger.h"

SimpleRouter& SimpleRouter::instance() {
    static SimpleRouter inst;
    return inst;
}

bool SimpleRouter::route(const HttpRequest& req, HttpResponse& resp) {
    for (const auto& route : m_routes) {
        if (route.method == req.methodString() && match(route.pattern, req.path())) {
            route.handler(req, resp);
            return true;
        }
    }
    return false;
}

bool SimpleRouter::match(const std::string& pattern, const std::string& path) {
    auto split = [](const std::string& s, char delim) {
        std::vector<std::string> parts;
        std::string cur;
        for (char c : s) {
            if (c == delim) {
                parts.push_back(cur);
                cur.clear();
            } else {
                cur += c;
            }
        }
        parts.push_back(cur);
        return parts;
    };

    auto pParts = split(pattern, '/');
    auto pathParts = split(path, '/');

    if (pParts.size() != pathParts.size()) return false;

    for (size_t i = 0; i < pParts.size(); ++i) {
        if (pParts[i].empty()) continue;
        if (pParts[i][0] == ':') {
        } else if (pParts[i] != pathParts[i]) {
            return false;
        }
    }
    return true;
}