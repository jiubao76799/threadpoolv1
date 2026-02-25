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

class ThreadPool {
public: 
    /// 构造函数，创建指定数量的工作线程
    ThreadPool(size_t threads, LogLevel logLevel = LogLevel::INFO,
              bool consoleLog = true, const std::string& logFile = "");

    //禁用拷贝构造函数和赋值运算符
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    //析构函数
    ~ThreadPool();

    //默认任务提交（中等优先级）
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
     
    // 带优先级的任务提交
    template<class F, class... Args>
    auto enqueueWithPriority(TaskPriority priority, F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // 带ID和描述的任务提交
    template<class F, class... Args>
    auto enqueueWithInfo(std::string taskId, std::string description,
                        TaskPriority priority, F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    //获取工作线程数量
    size_t getThreadCount() const ;

    //获取当前活跃的线程数量
    size_t getActiveThreadCount() const;

    //获取当前队列中等待线程数
    size_t getWaitingThreadCount() const;

    //获取当前队列中等待执行的任务数量
    size_t getTaskCount() ;

    //获取已完成的任务数量
    size_t getCompletedTaskCount() const;

    // 获取失败的任务数量
    size_t getFailedTaskCount() const;
    
    //动态调整线程池大小
    void resize(size_t threads);

    //暂停线程池
    void pause();

    //恢复线程池
    void resume();

    //等待所有任务完成
    void waitForTasks();

    //清空任务队列
    void clearTasks();

    // 获取性能报告
    std::string getMetricsReport() const;

    // 设置日志级别
    void setLogLevel(LogLevel level);

    //状态查询方法
    bool isStopped() const { return stop; }
    
private:
    //工作线程函数
    void workerThread(size_t id);
    
    // 创建任务函数
    template<class F, class... Args>
    auto createTaskFunction(std::shared_ptr<std::promise<typename std::result_of<F(Args...)>::type>> promise,
        F&& f, Args&&... args)
        -> std::function<void()>;
     
    // 记录任务提交日志
    void logTaskSubmission(const std::string& taskId, const std::string& description,
                          TaskPriority priority);
            
    //将队列改为集合，更适合存储要停止的线程ID
    std::unordered_set<size_t> threadsToStop;
    
    //工作线程容器
    std::vector<std::thread> workers;

    //任务队列(优先级队列)
    std::priority_queue<TaskInfo> tasks;

    // 任务ID映射表
    std::unordered_map<std::string, size_t> taskIdMap;

    //同步机制
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable waitCondition;

    //控制线程池状态
    std::atomic<bool> stop{false};
    std::atomic<bool> paused{false};

    // 日志系统
    Logger logger;

    // 性能指标
    ThreadPoolMetrics metrics;
};

//模板函数实现
#include "ThreadPool.inl"

#endif // THREAD_POOL_H
