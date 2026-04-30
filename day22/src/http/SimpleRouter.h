#ifndef SIMPLE_ROUTER_H
#define SIMPLE_ROUTER_H

#include <string>
#include <vector>
#include <functional>

class HttpRequest;
class HttpResponse;

struct Route {
    std::string method;
    std::string pattern;
    std::function<void(const HttpRequest&, HttpResponse&)> handler;
};

class SimpleRouter {
public:
    static SimpleRouter& instance();

    template<typename Handler>
    void get(const std::string& pattern, Handler&& handler) {
        addRoute("GET", pattern, std::forward<Handler>(handler));
    }

    template<typename Handler>
    void post(const std::string& pattern, Handler&& handler) {
        addRoute("POST", pattern, std::forward<Handler>(handler));
    }

    template<typename Handler>
    void put(const std::string& pattern, Handler&& handler) {
        addRoute("PUT", pattern, std::forward<Handler>(handler));
    }

    template<typename Handler>
    void del(const std::string& pattern, Handler&& handler) {
        addRoute("DELETE", pattern, std::forward<Handler>(handler));
    }

    bool route(const HttpRequest& req, HttpResponse& resp);
    size_t routeCount() const { return m_routes.size(); }

private:
    SimpleRouter() = default;

    template<typename Handler>
    void addRoute(const std::string& method, const std::string& pattern, Handler&& handler) {
        m_routes.push_back({method, pattern, std::forward<Handler>(handler)});
    }

    bool match(const std::string& pattern, const std::string& path);

    std::vector<Route> m_routes;
};

#endif