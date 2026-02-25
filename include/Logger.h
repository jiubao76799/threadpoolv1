#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>

// 日志级别枚举
enum class LogLevel {
    NONE = 0,    // 禁用所有日志
    ERROR = 1,   // 仅错误
    WARN = 2,    // 警告及以上
    INFO = 3,    // 信息及以上
    DEBUG = 4    // 调试及以上（所有日志）
};

// 日志系统类
class Logger {
public:
    // 构造函数
    Logger(LogLevel level = LogLevel::INFO, bool consoleOutput = true,
           const std::string& logFile = "");

    // 写日志
    void log(LogLevel msgLevel, const std::string& message);

    // 设置日志级别
    void setLevel(LogLevel newLevel);

    // 设置日志文件
    void setLogFile(const std::string& filename);

    // 启用/禁用控制台输出
    void setConsoleOutput(bool enable);

private:
    LogLevel level;
    bool consoleOutput;
    std::string logFile;
    std::mutex mutex;
};
#endif // LOGGER_H