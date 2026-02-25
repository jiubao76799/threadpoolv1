#include "Logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

Logger::Logger(LogLevel level, bool consoleOutput, const std::string& logFile)
    : level(level)
    , consoleOutput(consoleOutput)
    , logFile(logFile)
{}

void Logger::log(LogLevel msgLevel, const std::string& message) {
    if (level == LogLevel::NONE || msgLevel > level) return;
    
    std::lock_guard<std::mutex> lock(mutex);

    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream timestamp;
    std::tm timeinfo;

    // 跨平台处理
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&timeinfo, &now_time_t);
#else
    localtime_r(&now_time_t, &timeinfo);
#endif

    timestamp << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");

    // 日志级别字符串
    std::string levelStr;
    switch (msgLevel) {
    case LogLevel::ERROR: levelStr = "错误"; break;
    case LogLevel::INFO:  levelStr = "信息"; break;
    case LogLevel::DEBUG: levelStr = "调试"; break;
    default: levelStr = "未知";
    }

    // 格式化日志消息
    std::stringstream logMsg;
    logMsg << "[" << timestamp.str() << "] [" << levelStr << "] " << message;

    // 输出到控制台
    if (consoleOutput) {
        if (msgLevel == LogLevel::ERROR) {
            std::cerr << logMsg.str() << std::endl;
        }
        else {
            std::cout << logMsg.str() << std::endl;
        }
    }

    // 输出到文件
    if (!logFile.empty()) {
        try {
            std::ofstream file(logFile, std::ios::app);
            if (file.is_open()) {
                file << logMsg.str() << std::endl;
            }
        }
        catch (...) {
            // 忽略文件写入错误
            if (consoleOutput) {
                std::cerr << "无法写入日志文件: " << logFile << std::endl;
            }
        }
    }
}

void Logger::setLevel(LogLevel newLevel){
    level = newLevel;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    logFile = filename;
}

void Logger::setConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex);
    consoleOutput = enable;
}

