#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <unordered_set>
#include <unordered_map>

#include "Logger.h"
#include "TaskInfo.h"
#include "ThreadPoolMetrics.h"

// 线程池类
class ThreadPool {
public:
    // 构造函数，创建指定数量的工作线程
    ThreadPool(size_t threads, LogLevel logLevel = LogLevel::INFO,
              bool consoleLog = true, const std::string& logFile = "");

    // 禁用拷贝构造函数和赋值操作符
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 析构函数
    ~ThreadPool();

    // 默认任务提交（中等优先级，可选超时参数）
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // 带优先级的任务提交（可选超时参数）
    template<class F, class... Args>
    auto enqueueWithPriority(TaskPriority priority, std::chrono::milliseconds timeout,
                            F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // 批量提交任务（可选超时参数）
    template<class F>
    std::vector<std::future<void>> enqueueMany(const std::vector<F>& tasks,
                                              TaskPriority priority = TaskPriority::MEDIUM,
                                              std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // 带任务ID前缀的批量提交任务（可选超时参数）
    template<class F>
    std::vector<std::future<void>> enqueueManyWithIdPrefix(const std::string& idPrefix,
                                                          const std::string& descriptionPrefix,
                                                          const std::vector<F>& tasks,
                                                          TaskPriority priority = TaskPriority::MEDIUM,
                                                          std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    // 带ID、描述和可选超时的任务提交实现（主函数）
    template<class F, class... Args>
    auto enqueueWithInfo(std::string taskId, std::string description,
                        TaskPriority priority,
                        std::chrono::milliseconds timeout,
                        F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // 设置最大线程数
    void setMaxThreads(size_t max);

    // 获取最大线程数
    size_t getMaxThreads() const;

    // 获取工作线程数量
    size_t getThreadCount() const;

    // 获取当前活跃的线程数量
    size_t getActiveThreadCount() const;

    size_t getWaitingThreadCount() const;

    // 获取当前队列中等待执行的任务数量
    size_t getTaskCount();

    // 获取已完成的任务数量
    size_t getCompletedTaskCount() const;

    // 获取失败的任务数量
    size_t getFailedTaskCount() const;

    // 取消指定ID的任务
    bool cancelTask(const std::string& taskId);

    // 动态调整线程池大小
    void resize(size_t threads);

    // 暂停线程池
    void pause();

    // 恢复线程池
    void resume();

    // 等待所有任务完成
    void waitForTasks();

    // 清空任务队列
    void clearTasks();

    // 获取性能报告
    std::string getMetricsReport() const;

    // 设置日志级别
    void setLogLevel(LogLevel level);
    
    // 状态查询方法
    bool isStopped() const { return stop; }

    // 获取任务状态
    TaskStatus getTaskStatus(const std::string& taskId);

    // 获取任务状态字符串
    std::string getTaskStatusString(const std::string& taskId);

private:
    // 工作线程函数
    void workerThread(size_t id);

    // 工作线程功能
    TaskFetchResult getNextTask(size_t id, std::shared_ptr<TaskInfo>& taskPtr);
    void executeTask(size_t id, std::shared_ptr<TaskInfo> taskPtr);
    void executeTaskWithTimeout(std::shared_ptr<TaskInfo> taskPtr, bool& isTimeout);

    void handleTaskException(std::shared_ptr<TaskInfo> taskPtr, const std::string& errorMessage, bool isTimeout);
    void cleanupTask(std::shared_ptr<TaskInfo> taskPtr);
    void logTaskCompletion(size_t id, std::shared_ptr<TaskInfo> taskPtr, const std::chrono::nanoseconds& duration);

    // 创建任务函数
    template<class F, class... Args>
    auto createTaskFunction(std::shared_ptr<std::promise<typename std::result_of<F(Args...)>::type>> promise,
        F&& f, Args&&... args)
        -> std::function<void()>;
    
    // 记录任务提交日志
    void logTaskSubmission(const std::string& taskId, const std::string& description,
                          TaskPriority priority, std::chrono::milliseconds timeout);

    // 检查并创建任务信息
    std::shared_ptr<TaskInfo> checkAndCreateTaskInfo(
        const std::string& taskId,
        const std::string& description,
        TaskPriority priority,
        std::chrono::milliseconds timeout,
        std::function<void()> taskFunction);

    // 将队列改为集合，更适合存储要停止的线程ID
    std::unordered_set<size_t> threadsToStop;

    // 工作线程容器
    std::vector<std::thread> workers;

    // 任务队列（优先级队列）
    std::priority_queue<TaskInfo> tasks;

    // 任务ID映射表 - 从任务ID到任务信息的共享指针
    std::unordered_map<std::string, std::shared_ptr<TaskInfo>> taskIdMap;

    // 同步机制
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable waitCondition;

    // 控制线程池状态
    std::atomic<bool> stop{ false };
    std::atomic<bool> paused{ false };

    size_t maxThreads;  // 最大线程数限制

    // 日志系统
    Logger logger;

    // 性能指标
    ThreadPoolMetrics metrics;
};

// 模板函数实现
#include "ThreadPool.inl"

#endif // THREAD_POOL_H
