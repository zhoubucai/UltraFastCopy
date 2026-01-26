#pragma once
#include <string>
#include <fstream>

class Logger {
public:
    Logger(const std::string& filename = "");
    ~Logger();

    void log(const std::string& level, const std::string& msg, const char* file, int line);
    void info(const std::string& msg, const char* file, int line);
    void error(const std::string& msg, const char* file, int line);

private:
    std::ofstream fileStream;
    bool toFile;
};

// 宏定义，自动传递文件名和行号
#define LOG_INFO(msg)  g_log.info((msg), __FILE__, __LINE__)
#define LOG_ERROR(msg) g_log.error((msg), __FILE__, __LINE__)