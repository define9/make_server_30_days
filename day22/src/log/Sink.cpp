/**
 * @file Sink.cpp
 * @brief Sink实现
 */

#include "Sink.h"
#include <iostream>
#include <cstring>

void ConsoleSink::write(const std::string& formatted)
{
    std::cout << formatted << std::flush;
}

FileSink::FileSink(const std::string& filename)
{
    m_ofs.open(filename, std::ios::app);
    if (!m_ofs.is_open()) {
        std::cerr << "[FileSink] Failed to open file: " << filename << std::endl;
    }
}

FileSink::~FileSink()
{
    if (m_ofs.is_open()) {
        m_ofs.close();
    }
}

void FileSink::write(const std::string& formatted)
{
    if (m_ofs.is_open()) {
        m_ofs << formatted << std::flush;
    }
}
