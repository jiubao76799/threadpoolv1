#include "ThreadPool.h"
#include <iostream>

// 构造函数
ThreadPool::ThreadPool(size_t threads, LogLevel logLevel, bool consoleLog, const std::string& logFile)
    : logger(logLevel, consoleLog, logFile) {

    logger.log(LogLevel::INFO, "线程池创建，工作线程数: " + std::to_string(threads));

    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(
            [this, i] { this->workerThread(i); }
        );
    }
}

//析构函数 优雅地关闭线程池
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
        logger.log(LogLevel::INFO, "线程池正在关闭...");
    }

    condition.notify_all();

    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    logger.log(LogLevel::INFO, "线程池已关闭");
}

//获取工作线程数量
size_t ThreadPool::getThreadCount() const{
    return workers.size();
}

//获取当前队列中等待执行的任务数量
size_t ThreadPool::getTaskCount(){
    std::unique_lock<std::mutex> lock(queue_mutex);
    return tasks.size();
}

//获取已完成的任务数量
size_t ThreadPool::getCompletedTaskCount() const{
    return metrics.completedTasks;
}

//获取当前活跃的线程数量
size_t ThreadPool::getActiveThreadCount() const{
    return metrics.activeThreads;
}

// 获取当前等待的线程数量
size_t ThreadPool::getWaitingThreadCount() const {
    size_t totalThreads = getThreadCount();
    size_t activeCount = getActiveThreadCount();
    // 等待线程数 = 总线程数 - 活跃线程数
    return totalThreads - activeCount;
}

//获取失败的任务数量
size_t ThreadPool::getFailedTaskCount() const{
    return metrics.failedTasks;
}

//动态调整线程池大小
void ThreadPool::resize(size_t threads){
    std::unique_lock<std::mutex> lock(queue_mutex);

    // 如果线程池已停止，无法调整大小
    if(stop){
        throw std::runtime_error("resize on stopped ThreadPool");
    }

    //获取当前线程数
    size_t oldSize = workers.size();

    logger.log(LogLevel::INFO, "调整线程池大小: " + std::to_string(oldSize) + 
               " -> " + std::to_string(threads));

    //如果新的线程数大于当前线程数，添加新线程
    if(threads > oldSize){
        workers.reserve(threads);
        for(size_t i = oldSize; i < threads; ++i){
            workers.emplace_back([this,i]{this->workerThread(i);});
        }
    }
    //如果新的线程数小于当前线程数，我们需要减少线程
    else if (threads < oldSize){
        //清空之前可能存在的待停止线程集合
        threadsToStop.clear();

        //添加要停止的线程ID(索引)到集合中
        for(size_t i = threads; i < oldSize; ++i){
            threadsToStop.insert(i);
        }

        //解锁并通知
        lock.unlock();
        condition.notify_all();

        //等待线程结束
        for(size_t i = threads; i < oldSize; ++i){
            if(workers[i].joinable()){
                workers[i].join();
            }
        }

        //重新获取锁和调整线程容器大小
        lock.lock();
        workers.resize(threads);
    }

}

//暂停线程池
void ThreadPool::pause(){
    std::unique_lock<std::mutex> lock(queue_mutex);
    paused = true;
    logger.log(LogLevel::INFO, "线程池已暂停");
}

//恢复线程池
void ThreadPool::resume(){
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        paused = false;
        logger.log(LogLevel::INFO, "线程池已恢复");
    }
    condition.notify_all();
}

// 等待所有任务完成
void ThreadPool::waitForTasks() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    logger.log(LogLevel::INFO, "等待所有任务完成...");
    waitCondition.wait(lock, [this] {
        return (tasks.empty() && metrics.activeThreads == 0) || stop;
    });
    logger.log(LogLevel::INFO, "所有任务已完成");
}

// 清空任务队列
void ThreadPool::clearTasks() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    size_t taskCount = tasks.size();
    
    // 清空任务队列和ID映射表
    std::priority_queue<TaskInfo> emptyQueue;
    std::swap(tasks, emptyQueue);
    taskIdMap.clear();
    
    logger.log(LogLevel::INFO, "清空任务队列: " + std::to_string(taskCount) + " 个任务被移除");
}

// 获取性能报告
std::string ThreadPool::getMetricsReport() const {
    return metrics.getReport();
}

// 设置日志级别
void ThreadPool::setLogLevel(LogLevel level) {
    logger.setLevel(level);
}

// 记录任务提交日志
void ThreadPool::logTaskSubmission(const std::string& taskId, const std::string& description,
                                   TaskPriority priority) {
    std::string priorityStr = priorityToString(priority);
    
    if (!taskId.empty() || !description.empty()) {
        logger.log(LogLevel::DEBUG, "提交任务 " + taskId + " (" + description + 
                   ") 优先级: " + priorityStr);
    } else {
        logger.log(LogLevel::DEBUG, "提交" + priorityStr + "优先级任务");
    }
}

//工作线程函数 - 线程的工作逻辑
void ThreadPool::workerThread(size_t id){
    logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 启动");

    while(true){
        TaskInfo task{nullptr};  // 使用默认构造的空任务
        bool hasTask = false;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            //等待直到有任务或者线程池停止或需要退出
            condition.wait(lock, [this,id]{
                return this->stop ||
                    (!this->paused && !this->tasks.empty()) ||
                    (this->threadsToStop.find(id) != this->threadsToStop.end());
            });

            //唤醒后的第一件事是检查是否应该退出
            if(this->stop ){
                 logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 停止（线程池关闭）");
                return;
            }

            //检查是否要终止当前线程
            if(this->threadsToStop.find(id) != this->threadsToStop.end()){
                this->threadsToStop.erase(id);
                logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 停止（线程池调整大小）");
                return;
            }

            //最后检查是否有任务可执行
            if(!this->paused &&!this->tasks.empty()){
                 // 优先级队列不支持直接移动元素，需要做一个拷贝
                task = this->tasks.top();
                this->tasks.pop();
                hasTask = true;

                // 记录日志
                std::string taskDesc = task.taskId.empty() ? "匿名任务" : "任务 " + task.taskId;
                if (!task.description.empty()) {
                    taskDesc += " (" + task.description + ")";
                }
                logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 开始执行 " + taskDesc);
            }  
        }

        // 执行任务
        if (hasTask && task.task) {
            // 更新任务状态和性能指标
            task.status = TaskStatus::RUNNING;
            metrics.activeThreads++;
            metrics.updateActiveThreads(metrics.activeThreads);

            auto startTime = std::chrono::steady_clock::now();

            try {
                task.task();
                task.status = TaskStatus::COMPLETED;
                metrics.completedTasks++;
            }
            catch (const std::exception& e) {
                task.status = TaskStatus::FAILED;
                task.errorMessage = e.what();
                metrics.failedTasks++;
                logger.log(LogLevel::ERROR, "任务异常: " + std::string(e.what()));
            }
            catch (...) {
                task.status = TaskStatus::FAILED;
                task.errorMessage = "未知异常";
                metrics.failedTasks++;
                logger.log(LogLevel::ERROR, "任务发生未知异常");
            }

            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
            metrics.addTaskTime(duration.count());

            metrics.activeThreads--;
            waitCondition.notify_all();

            // 记录任务完成日志
            std::string taskDesc = task.taskId.empty() ? "匿名任务" : "任务 " + task.taskId;
            std::string statusStr = taskStatusToString(task.status);

            logger.log(LogLevel::DEBUG, 
                      "工作线程 " + std::to_string(id) + " " + statusStr + " " + taskDesc + 
                      " (用时: " + std::to_string(duration.count() / 1000000.0) + "ms)");
        }
    }
}

