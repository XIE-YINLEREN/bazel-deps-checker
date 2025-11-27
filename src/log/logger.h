#pragma once

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>


enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    LOG_ERROR = 4,
    FATAL = 5,
    OFF = 6
};

class Logger {
public:
    // 获取单例实例
    static Logger& getInstance();
    
    // 删除拷贝构造和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 配置方法
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    void enableConsoleOutput(bool enable);
    void enableFileOutput(bool enable);
    
    // 日志输出方法
    void log(LogLevel level, const std::string& message, 
             const char* file = "", int line = 0, const char* function = "");
    
    // 便捷宏使用的内部方法
    void logWithStream(LogLevel level, const std::string& message, 
                      const char* file, int line, const char* function);

private:
    Logger();
    ~Logger();
    
    std::string getCurrentTime();
    std::string levelToString(LogLevel level);
    std::string getColorCode(LogLevel level);
    std::string resetColor();
    
    LogLevel currentLevel_;
    std::ofstream fileStream_;
    std::string filename_;
    bool consoleOutput_;
    bool fileOutput_;
    std::mutex mutex_;
    
    // 控制台颜色支持
    bool colorSupport_;
};


// 便捷日志宏
#define LOG_TRACE(message) \
    Logger::getInstance().logWithStream(LogLevel::TRACE, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_DEBUG(message) \
    Logger::getInstance().logWithStream(LogLevel::DEBUG, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_INFO(message) \
    Logger::getInstance().logWithStream(LogLevel::INFO, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_WARN(message) \
    Logger::getInstance().logWithStream(LogLevel::WARN, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_ERROR(message) \
    Logger::getInstance().logWithStream(LogLevel::LOG_ERROR, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_FATAL(message) \
    Logger::getInstance().logWithStream(LogLevel::FATAL, message, __FILE__, __LINE__, __FUNCTION__)

    
#endif // LOGGER_H

