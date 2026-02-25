#ifndef TASK_INFO_H
#define TASK_INFO_H

#include <functional>
#include <string>
#include <chrono>

//任务优先级枚举
enum class TaskPriority{
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

//任务执行状态枚举
enum class TaskStatus {
    WAITING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELED
};

//任务信息结构
struct TaskInfo{
    std::function<void()> task;
    TaskPriority priority;
    std::chrono::steady_clock::time_point submitTime;
    std::string taskId;
    std::string description;
    TaskStatus status{ TaskStatus::WAITING };
    std::string errorMessage;

    //构造函数
    TaskInfo(std::function<void()> t = nullptr,
             TaskPriority p = TaskPriority::MEDIUM,
            std::string id  = "",
            std::string desc = "");

    //比较运算符重载,用于优先队列
    bool operator<(const TaskInfo& other) const;                
};

//辅助函数:讲任务状态转换为字符串
std::string taskStatusToString(TaskStatus status);

//辅助函数:将任务优先级转为字符串
std::string priorityToString(TaskPriority priority);

#endif // TASK_INFO_H

