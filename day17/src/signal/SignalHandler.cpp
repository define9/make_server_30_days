#include "SignalHandler.h"
#include "log/Logger.h"
#include <iostream>
#include <unistd.h>
#include <sys/time.h>

SignalHandler& SignalHandler::instance()
{
    static SignalHandler instance;
    return instance;
}

SignalHandler::SignalHandler()
    : m_command(SignalCommand::NONE)
    , m_shuttingDown(false)
    , m_started(false)
{
    m_signalMap[SIGINT]  = SignalCommand::SHUTDOWN;
    m_signalMap[SIGTERM] = SignalCommand::SHUTDOWN;
    m_signalMap[SIGHUP]  = SignalCommand::RELOAD;
}

SignalHandler::~SignalHandler()
{
    if (m_started.load()) {
        // 恢复默认信号处理
        sigaction(SIGINT, &m_oldSigINT, nullptr);
        sigaction(SIGTERM, &m_oldSigTERM, nullptr);
        sigaction(SIGHUP, &m_oldSigHUP, nullptr);
        sigaction(SIGPIPE, &m_oldSigPIPE, nullptr);
    }
}

void SignalHandler::start()
{
    if (m_started.load()) {
        return;
    }
    m_started.store(true);
    installHandlers();
    LOG_INFO("Signal handler started");
}

void SignalHandler::wait()
{
    /**
     * 阻塞等待直到收到关闭信号
     * 
     * 实现原理：
     *   pause() 是一个古老的系统调用，它会挂起进程直到任意信号到达。
     *   当 SIGINT/SIGTERM 被捕获并处理后，processSignal() 会设置 m_shuttingDown = true，
     *   此时循环条件不满足，wait() 才会返回。
     * 
     * 与 waitForExit() 的区别：
     *   - wait()       等待"信号"（外部事件，Ctrl+C/kill命令）
     *   - waitForExit()等待"线程"（内部工作线程全部执行完毕）
     * 
     * 典型调用顺序：
     *   signalHandler.wait();           // 1. 等信号（被中断才返回）
     *   reactorPool.waitForExit();      // 2. 等所有reactor线程退出
     *   threadPool.waitForIdle();        // 3. 等所有task线程空闲
     * 
     * @see ReactorPool::waitForExit()
     * @see ThreadPool::waitForIdle()
     */
    while (!m_shuttingDown.load()) {
        pause();  // 等待信号（进程在此休眠，不消耗CPU）
    }
}

void SignalHandler::onShutdown(std::function<void()> callback)
{
    m_shutdownCallback = callback;
}

void SignalHandler::onReload(std::function<void()> callback)
{
    m_reloadCallback = callback;
}

SignalCommand SignalHandler::getCommand() const
{
    return m_command.load();
}

void SignalHandler::resetCommand()
{
    m_command.store(SignalCommand::NONE);
}

bool SignalHandler::isShuttingDown() const
{
    return m_shuttingDown.load();
}

void SignalHandler::signalShutdown()
{
    // 给当前进程发送SIGTERM
    kill(getpid(), SIGTERM);
}

void SignalHandler::handleSignal(int sig)
{
    instance().processSignal(sig);
}

void SignalHandler::processSignal(int sig)
{
    auto it = m_signalMap.find(sig);
    if (it == m_signalMap.end()) {
        return;
    }

    SignalCommand cmd = it->second;
    m_command.store(cmd);

    switch (cmd) {
        case SignalCommand::SHUTDOWN:
            if (!m_shuttingDown.load()) {
                LOG_INFO("Received SIGINT/SIGTERM, initiating shutdown...");
                setShuttingDown();
                if (m_shutdownCallback) {
                    m_shutdownCallback();
                }
            }
            break;

        case SignalCommand::RELOAD:
            LOG_INFO("Received SIGHUP, reloading configuration...");
            if (m_reloadCallback) {
                m_reloadCallback();
            }
            break;

        default:
            break;
    }
}

void SignalHandler::installHandlers()
{
    /**
     * 安装所有信号处理器
     * 
     * 调用时机：在 SignalHandler::start() 中调用
     * 调用位置：main() → signalHandler.start() → installHandlers()
     * 
     * 信号安装顺序：
     *   1. SIGINT  - Ctrl+C 终止
     *   2. SIGTERM - kill命令优雅终止
     *   3. SIGHUP  - 配置重载
     *   4. SIGPIPE - 忽略（防止向关闭的socket写数据导致崩溃）
     * 
     * SA_RESTART 标志：自动重启被信号中断的系统调用，
     * 例如 read()/write()/accept() 等系统调用被信号打断后会自动重试，
     * 避免开发者处理 EINTR 错误码。
     */
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_flags |= SA_RESTART;  // 自动重启被信号中断的系统调用

    // 安装SIGINT处理器
    sigaction(SIGINT, &sa, &m_oldSigINT);

    // 安装SIGTERM处理器
    sigaction(SIGTERM, &sa, &m_oldSigTERM);

    // 安装SIGHUP处理器
    sigaction(SIGHUP, &sa, &m_oldSigHUP);

    // 忽略SIGPIPE，防止向已关闭的socket写数据导致进程崩溃
    struct sigaction ignoreSa;
    ignoreSa.sa_handler = SIG_IGN;
    sigemptyset(&ignoreSa.sa_mask);
    ignoreSa.sa_flags = 0;
    sigaction(SIGPIPE, &ignoreSa, &m_oldSigPIPE);

    LOG_DEBUG("Installed handlers for SIGINT, SIGTERM, SIGHUP, SIGPIPE");
}

void SignalHandler::setShuttingDown()
{
    m_shuttingDown.store(true);
}