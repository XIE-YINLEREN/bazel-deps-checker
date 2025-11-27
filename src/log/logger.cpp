#include "logger.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

Logger::Logger() 
    : currentLevel_(LogLevel::INFO)
    , consoleOutput_(true)
    , fileOutput_(false)
    , colorSupport_(true) {
    
    // 检查是否支持颜色输出
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    colorSupport_ = GetConsoleMode(hConsole, &mode) && 
                   SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    colorSupport_ = isatty(fileno(stdout));
#endif
}

Logger::~Logger() {
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentLevel_ = level;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    filename_ = filename;
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
    if (!filename.empty()) {
        fileStream_.open(filename, std::ios::app);
        fileOutput_ = fileStream_.is_open();
    }
}

void Logger::enableConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    consoleOutput_ = enable;
}

void Logger::enableFileOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    fileOutput_ = enable;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::LOG_ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::getColorCode(LogLevel level) {
    if (!colorSupport_) return "";
    
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";  // 白色
        case LogLevel::DEBUG: return "\033[36m";  // 青色
        case LogLevel::INFO:  return "\033[32m";  // 绿色
        case LogLevel::WARN:  return "\033[33m";  // 黄色
        case LogLevel::LOG_ERROR: return "\033[31m";  // 红色
        case LogLevel::FATAL: return "\033[35m";  // 洋红色
        default: return "";
    }
}

std::string Logger::resetColor() {
    return colorSupport_ ? "\033[0m" : "";
}

void Logger::log(LogLevel level, const std::string& message, 
                const char* file, int line, const char* function) {
    if (level < currentLevel_ || level == LogLevel::OFF) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timeStr = getCurrentTime();
    std::string levelStr = levelToString(level);
    std::string colorCode = getColorCode(level);
    std::string resetCode = resetColor();
    
    // 提取文件名（不含路径）
    std::string filename = file;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }
    
    std::ostringstream logLine;
    
    // 控制台输出（带颜色）
    if (consoleOutput_) {
        logLine << colorCode << "[" << timeStr << "] "
                << "[" << levelStr << "] "
                << "[" << filename << ":" << line << ":" << function << "] "
                << message << resetCode;
        std::cout << logLine.str() << std::endl;
    }
    
    // 文件输出（无颜色）
    if (fileOutput_ && fileStream_.is_open()) {
        logLine.str(""); // 清空流
        logLine << "[" << timeStr << "] "
                << "[" << levelStr << "] "
                << "[" << filename << ":" << line << ":" << function << "] "
                << message;
        fileStream_ << logLine.str() << std::endl;
        fileStream_.flush();
    }
}

void Logger::logWithStream(LogLevel level, const std::string& message, 
                          const char* file, int line, const char* function) {
    log(level, message, file, line, function);
}
