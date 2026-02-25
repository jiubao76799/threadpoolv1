#include "TaskInfo.h"
#include <thread>

TaskInfo::TaskInfo::TaskInfo(std::function<void()> t,
                 TaskPriority p,
                 std::string id,
                 std::string desc)
    : task(std::move(t))
    , priority(p)
    , submitTime(std::chrono::steady_clock::now())
    , taskId(std::move(id))
    , description(std::move(desc))
{
    // 为了避免时间精度问题，添加微小的延迟确保时间戳不同
   // std::this_thread::sleep_for(std::chrono::nanoseconds(1));
}

bool TaskInfo::operator<(const TaskInfo& other) const {
    // 首先按优先级比较
    if (priority != other.priority) {
        return priority < other.priority;  // 较高的优先级在前
    }
    // 同优先级按提交时间排序（先进先出）
    return submitTime > other.submitTime;  // 较早提交的在前
}

std::string taskStatusToString(TaskStatus status) {
    switch (status) {
    case TaskStatus::WAITING:   return "等待中";
    case TaskStatus::RUNNING:   return "正在执行";
    case TaskStatus::COMPLETED: return "已完成";
    case TaskStatus::FAILED:    return "失败";
    case TaskStatus::CANCELED:  return "已取消";
    default:                    return "未知状态";
    }
}

std::string priorityToString(TaskPriority priority) {
    switch (priority) {
    case TaskPriority::LOW:      return "低";
    case TaskPriority::MEDIUM:   return "中";
    case TaskPriority::HIGH:     return "高";
    case TaskPriority::CRITICAL: return "关键";
    default:                     return "未知";
    }
}