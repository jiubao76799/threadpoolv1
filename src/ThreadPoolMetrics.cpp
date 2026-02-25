#include "ThreadPoolMetrics.h"
#include <sstream>
#include <iomanip>

ThreadPoolMetrics::ThreadPoolMetrics() : startTime(std::chrono::steady_clock::now()) {}

void ThreadPoolMetrics::updateQueueSize(size_t size) {
    size_t currentPeak = peakQueueSize.load();
    while (size > currentPeak && !peakQueueSize.compare_exchange_weak(currentPeak, size)) {
        // 如果更新失败，currentPeak会被更新为当前值，然后重试
    }
}

void ThreadPoolMetrics::updateActiveThreads(size_t count) {
    activeThreads.store(count);
    size_t currentPeak = peakThreads.load();
    while (count > currentPeak && !peakThreads.compare_exchange_weak(currentPeak, count)) {
        // 如果更新失败，currentPeak会被更新为当前值，然后重试
    }
}

void ThreadPoolMetrics::addTaskTime(uint64_t timeNs) {
    totalTaskTimeNs.fetch_add(timeNs);
}

double ThreadPoolMetrics::getAverageTaskTime() const {
    size_t completed = completedTasks.load();
    if (completed == 0) return 0.0;
    return static_cast<double>(totalTaskTimeNs.load()) / completed / 1000000.0;
}

double ThreadPoolMetrics::getUptime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - startTime).count();
}

double ThreadPoolMetrics::getThroughput() const {
    double uptime = getUptime();
    if (uptime <= 0.0) return 0.0;
    return static_cast<double>(completedTasks.load()) / uptime;
}

std::string ThreadPoolMetrics::getReport() const {
    std::stringstream ss;
    ss << "线程池性能报告:" << std::endl;
    ss << "  运行时间: " << getUptime() << " 秒" << std::endl;
    ss << "  总任务数: " << totalTasks.load() << std::endl;
    ss << "  已完成任务数: " << completedTasks.load() << std::endl;
    ss << "  失败任务数: " << failedTasks.load() << std::endl;
    ss << "  当前活跃线程数: " << activeThreads.load() << std::endl;
    ss << "  峰值活跃线程数: " << peakThreads.load() << std::endl;
    ss << "  峰值队列大小: " << peakQueueSize.load() << std::endl;
    ss << "  平均任务执行时间: " << getAverageTaskTime() << " 毫秒" << std::endl;
    ss << "  任务吞吐量: " << getThroughput() << " 任务/秒" << std::endl;
    return ss.str();
}