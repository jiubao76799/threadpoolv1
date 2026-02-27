#ifndef TASK_INFO_H
#define TASK_INFO_H

#include <functional>
#include <string>
#include <chrono>

// 任务优先级枚举
enum class TaskPriority {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// 获取任务的结果枚举
enum class TaskFetchResult {
    SHOULD_EXIT,  // 线程应该退出
    NO_TASK,      // 没有任务，但应该继续运行
    HAS_TASK      // 成功获取了任务
};

// 任务执行状态枚举
enum class TaskStatus {
    WAITING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELED,
    NOT_FOUND    // 任务不存在
};

// 任务信息结构
struct TaskInfo {
    std::function<void()> task;
    TaskPriority priority;
    std::chrono::steady_clock::time_point submitTime;
    std::string taskId;
    std::string description;
    TaskStatus status{ TaskStatus::WAITING };
    std::string errorMessage;
    std::chrono::milliseconds timeout{ 0 };  // 任务超时时间（毫秒），0表示无超时限制

    // 构造函数
    TaskInfo(std::function<void()> t = nullptr,
             TaskPriority p = TaskPriority::MEDIUM,
             std::string id = "",
             std::string desc = "",
             std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // 比较运算符重载，用于优先级队列
    bool operator<(const TaskInfo& other) const;
};

// 辅助函数：将任务状态转为字符串
std::string taskStatusToString(TaskStatus status);

// 辅助函数：将任务优先级转为字符串
std::string priorityToString(TaskPriority priority);

#endif // TASK_INFO_H
