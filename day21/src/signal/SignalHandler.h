/**
 * @file SignalHandler.h
 * @brief 信号处理器 - 统一管理进程信号处理
 * 
 * ==================== 信号处理架构 ====================
 * 
 * 信号是进程间通信的一种方式，用于响应某些事件：
 * 
 *     ┌─────────────────────────────────────────────┐
 *     │              SignalHandler                  │
 *     │  ┌───────────────────────────────────────┐  │
 *     │  │         Signal-to-Action Map           │  │
 *     │  │   SIGINT  → shutdown()                 │  │
 *     │  │   SIGTERM → shutdown()                 │  │
 *     │  │   SIGHUP  → reload()                  │  │
 *     │  │   SIGPIPE → ignore                    │  │
 *     │  └───────────────────────────────────────┘  │
 *     │                      │                     │
 *     │              ┌───────┴───────┐             │
 *     │              ▼               ▼             │
 *     │      ┌───────────┐   ┌───────────┐       │
 *     │      │  Callbacks │   │   atExit   │       │
 *     │      │  (handlers)│   │  callbacks │       │
 *     │      └───────────┘   └───────────┘       │
 *     └─────────────────────────────────────────────┘
 * 
 * ==================== 为什么需要SignalHandler ====================
 * 
 * 1. 标准signal()的局限性：
 *    - 行为不确定（System V vs BSD）
 *    - 信号处理函数中能调用的函数有限
 *    - 无法轻松注册多个处理程序
 * 
 * 2. 我们需要：
 *    - 可靠的信号处理（使用sigaction）
 *    - 优雅退出流程
 *    - 支持配置reload
 *    - 线程安全
 * 
 * ==================== 使用方式 ====================
 * 
 * @code
 * // 注册退出回调
 * SignalHandler::instance().onShutdown([]() {
 *     LOG_INFO("Shutting down...");
 *     server.stop();
 * });
 * 
 * // 注册reload回调
 * SignalHandler::instance().onReload([]() {
 *     LOG_INFO("Reloading config...");
 *     config.reload();
 * });
 * 
 * // 启动信号处理
 * SignalHandler::instance().start();
 * 
 * // 阻塞等待信号
 * SignalHandler::instance().wait();
 * @endcode
 */

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <functional>
#include <map>
#include <signal.h>
#include <atomic>

/**
 * @brief 信号处理命令
 */
enum class SignalCommand {
    NONE,       // 无命令
    SHUTDOWN,   // 关闭服务器
    RELOAD,     // 重新加载配置
    TERMINATE   // 终止进程
};

/**
 * @brief 信号处理器类（单例）
 * 
 * 统一管理进程的所有信号处理，提供：
 * - SIGINT/SIGTERM：优雅关闭
 * - SIGHUP：配置重载
 * - SIGPIPE：忽略（防止write到关闭的socket）
 */
class SignalHandler {
public:
    /**
     * @brief 获取单例实例
     */
    static SignalHandler& instance();
    
    // 禁用拷贝
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;
    
    /**
     * @brief 启动信号处理
     * 
     * 安装信号处理器，开始处理信号。
     * 必须在所有组件初始化完成后调用。
     */
    void start();
    
    /**
     * @brief 阻塞等待信号
     * 
     * 等待信号被触发并处理。
     * 通常在服务器启动后调用。
     */
    void wait();
    
    /**
     * @brief 注册关闭回调
     * @param callback 关闭时执行的回调函数
     */
    void onShutdown(std::function<void()> callback);
    
    /**
     * @brief 注册配置重载回调
     * @param callback 重载时执行的回调函数
     */
    void onReload(std::function<void()> callback);
    
    /**
     * @brief 获取最新信号命令
     */
    SignalCommand getCommand() const;
    
    /**
     * @brief 重置信号命令
     */
    void resetCommand();
    
    /**
     * @brief 检查是否收到关闭信号
     */
    bool isShuttingDown() const;
    
    /**
     * @brief 发送关闭信号（给自己）
     */
    void signalShutdown();
    
private:
    SignalHandler();
    ~SignalHandler();
    
    /**
     * @brief 信号处理函数（静态）
     */
    static void handleSignal(int sig);
    
    /**
     * @brief 处理单个信号
     */
    void processSignal(int sig);
    
    /**
     * @brief 安装所有信号处理器
     */
    void installHandlers();
    
    /**
     * @brief 标记关闭状态
     */
    void setShuttingDown();
    
    // 信号到命令的映射
    std::map<int, SignalCommand> m_signalMap;
    
    // 回调函数
    std::function<void()> m_shutdownCallback;
    std::function<void()> m_reloadCallback;
    
    // 当前命令
    std::atomic<SignalCommand> m_command;
    
    // 是否正在关闭
    std::atomic<bool> m_shuttingDown;
    
    // 是否已启动
    std::atomic<bool> m_started;
    
    // 原始sigaction保存
    struct sigaction m_oldSigINT;
    struct sigaction m_oldSigTERM;
    struct sigaction m_oldSigHUP;
    struct sigaction m_oldSigPIPE;
};

#endif // SIGNAL_HANDLER_H