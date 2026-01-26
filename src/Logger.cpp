#include "../include/Logger.h"
#include <iostream>
#include <ctime>

Logger::Logger(const std::string& filename) : toFile(false) {
    if (!filename.empty()) {
        fileStream.open(filename, std::ios::app);
        if (fileStream.is_open()) {
            toFile = true;
        }
    }
}

Logger::~Logger() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

void Logger::log(const std::string& level, const std::string& msg, const char* file, int line) {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    std::string output = "[" + std::string(buf) + "] [" + level + "] " +
                         file + std::string(":") + std::to_string(line) + " " + msg + "\n";
    std::cout << output;
    if (toFile && fileStream.is_open()) {
        fileStream << output;
        fileStream.flush();
    }
}

void Logger::info(const std::string& msg, const char* file, int line) {
    log("INFO", msg, file, line);
}

void Logger::error(const std::string& msg, const char* file, int line) {
    log("ERROR", msg, file, line);
}