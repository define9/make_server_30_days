#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

volatile sig_atomic_t g_running = 1;

void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <服务器IP> <端口>" << std::endl;
        return 1;
    }

    const char* server_ip = argv[1];
    int port = std::atoi(argv[2]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "无法创建 socket" << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "无法连接到 " << server_ip << ":" << port << std::endl;
        close(sock);
        return 1;
    }

    std::cout << "已连接到 " << server_ip << ":" << port << std::endl;
    std::cout << "输入消息并按回车发送，Ctrl+C 退出" << std::endl;

    while (g_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);

        int max_fd = std::max(STDIN_FILENO, sock) + 1;
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ready = select(max_fd, &read_fds, nullptr, nullptr, &tv);
        if (ready < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ready == 0) continue;

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::string input;
            if (std::getline(std::cin, input)) {
                input += '\n';
                send(sock, input.c_str(), input.size(), 0);
            } else {
                break;
            }
        }

        if (FD_ISSET(sock, &read_fds)) {
            char buffer[1024];
            ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                std::cout << "[服务器]: " << buffer;
            } else {
                break;
            }
        }
    }

    close(sock);
    return 0;
}