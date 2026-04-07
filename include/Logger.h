#pragma once
#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    //构造函数
    Logger(const std::string& filename = "");
    //析构函数
    ~Logger();
    //设置日志输出等级
    void SetLogLevel(LogLevel level);
    //日志输出接口
    void Log(const std::string& level, const std::string& msg, const char* file, int line);
    //各等级路由函数，内部会进行等级阈值校验
    void Debug(const std::string& msg, const char* file, int line);
    void Info(const std::string& msg, const char* file, int line);
    void Warning(const std::string& msg, const char* file, int line);
    void Error(const std::string& msg, const char* file, int line);

private:
    std::ofstream _fileStream;
    bool _toFile;//标识当前是否成功打开了日志文件并需要进行文件写入
    LogLevel _currentLevel;
    std::mutex _mutex;
};

extern Logger g_log;

//宏定义，自动传递文件名和行号
#define LOG_DEBUG(msg) g_log.Debug((msg), __FILE__, __LINE__)
#define LOG_INFO(msg)  g_log.Info((msg), __FILE__, __LINE__)
#define LOG_WARNING(msg) g_log.Warning((msg), __FILE__, __LINE__)
#define LOG_ERROR(msg) g_log.Error((msg), __FILE__, __LINE__)