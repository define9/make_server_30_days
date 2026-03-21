/**
 * @file Sink.h
 * @brief 日志输出目标抽象 - Sink模式
 */

#ifndef SINK_H
#define SINK_H

#include <string>
#include <memory>
#include <fstream>

/**
 * @brief Sink基类 - 日志输出目标抽象
 * 
 * Sink模式将日志格式化与输出分离，便于扩展多种输出目标。
 * 
 * 现有实现：
 * - ConsoleSink - 输出到控制台
 * - FileSink - 输出到文件
 */
class Sink {
public:
    virtual ~Sink() = default;
    
    /**
     * @brief 写入日志
     * @param formatted 格式化后的日志字符串
     */
    virtual void write(const std::string& formatted) = 0;
};

/**
 * @brief 控制台输出
 */
class ConsoleSink : public Sink {
public:
    void write(const std::string& formatted) override;
};

/**
 * @brief 文件输出
 */
class FileSink : public Sink {
public:
    explicit FileSink(const std::string& filename);
    ~FileSink() override;
    
    void write(const std::string& formatted) override;
    
private:
    std::ofstream m_ofs;
};

#endif // SINK_H
