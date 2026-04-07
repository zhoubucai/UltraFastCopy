#include "../include/Logger.h"
#include <iostream>
#include <ctime>
#include <filesystem>

//构造函数，默认不写入文件 (_toFile=false)，并将默认日志等级设置为 INFO
Logger::Logger(const std::string& filename) : _toFile(false), _currentLevel(LogLevel::INFO) {
    if (!filename.empty()) {
        //先判断日志文件目录是否存在，不存在则级联创建
        std::filesystem::path logPath(filename);
        std::filesystem::path logDir=logPath.parent_path();
        if(!logDir.empty() && !std::filesystem::exists(logDir)){
            std::error_code ec;
            std::filesystem::create_directories(logDir, ec);
            if(ec){
                std::cout<<"[WARNING] 日志目录创建失败: "<<logDir.string()<<std::endl;
            }
        }
        _fileStream.open(filename, std::ios::app);
        if (_fileStream.is_open()) {
            _toFile = true;
        }
        else{
            std::cout<<"[WARNING] 无法打开日志文件进行写入: "<<filename<<std::endl;
        }
    }
}
//析构函数
Logger::~Logger() {
    if (_fileStream.is_open()) {
        _fileStream.close();
    }
}
//设置日志输出等级
void Logger::SetLogLevel(LogLevel level){
    _currentLevel = level;
}
//日志输出接口
void Logger::Log(const std::string& level, const std::string& msg, const char* file, int line) {
    std::lock_guard<std::mutex> lock(_mutex);
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    std::string output = "[" + std::string(buf) + "] [" + level + "] " +
                         file + std::string(":") + std::to_string(line) + " " + msg + "\n";
    std::cout << output;
    if (_toFile && _fileStream.is_open()) {
        _fileStream << output;
        _fileStream.flush();
    }
}
//各等级路由函数
void Logger::Debug(const std::string& msg, const char* file, int line) {
    if(_currentLevel <= LogLevel::DEBUG){
        Log("DEBUG", msg, file, line);
    }
}
void Logger::Info(const std::string& msg, const char* file, int line) {
    if(_currentLevel <= LogLevel::INFO){
        Log("INFO", msg, file, line);
    }
}
void Logger::Warning(const std::string& msg, const char* file, int line) {
    if(_currentLevel <= LogLevel::WARNING){
        Log("WARNING", msg, file, line);
    }
}
void Logger::Error(const std::string& msg, const char* file, int line) {
    if(_currentLevel <= LogLevel::ERROR){
        Log("ERROR", msg, file, line);
    }
}