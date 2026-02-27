#ifndef THREAD_POOL_METRICS_H
#define THREAD_POOL_METRICS_H

#include <atomic>
#include <chrono>
#include <string>

// 线程池性能指标
struct ThreadPoolMetrics {
    std::atomic<size_t> totalTasks{ 0 };           // 总任务数
    std::atomic<size_t> completedTasks{ 0 };       // 已完成任务数
    std::atomic<size_t> failedTasks{ 0 };          // 失败任务数
    std::atomic<size_t> canceledTasks{ 0 };        // 取消的任务数
    std::atomic<size_t> timedOutTasks{ 0 };        // 超时的任务数
    std::atomic<size_t> activeThreads{ 0 };        // 活跃线程数
    std::atomic<size_t> peakThreads{ 0 };          // 峰值线程数
    std::atomic<size_t> peakQueueSize{ 0 };        // 峰值队列大小
    std::chrono::steady_clock::time_point startTime;  // 线程池启动时间
    std::atomic<uint64_t> totalTaskTimeNs{ 0 };    // 总任务执行时间（纳秒）

    // 构造函数
    ThreadPoolMetrics();

    // 更新队列大小并记录峰值
    void updateQueueSize(size_t size);

    // 更新活跃线程数并记录峰值
    void updateActiveThreads(size_t count);

    // 添加任务执行时间
    void addTaskTime(uint64_t timeNs);

    // 获取平均任务执行时间（毫秒）
    double getAverageTaskTime() const;

    // 获取线程池运行时间（秒）
    double getUptime() const;

    // 获取任务吞吐量（每秒任务数）
    double getThroughput() const;

    // 获取性能报告
    std::string getReport() const;
};

#endif // THREAD_POOL_METRICS_H
