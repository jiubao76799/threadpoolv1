#include "ThreadPool.h"
#include <iostream>

// 构造函数
ThreadPool::ThreadPool(size_t threads, LogLevel logLevel, bool consoleLog, const std::string& logFile)
    : maxThreads(std::max(threads * 2, (size_t)16))
    , logger(logLevel, consoleLog, logFile) {

    // 确保初始线程数不超过最大线程数
    threads = std::min(threads, maxThreads);

    logger.log(LogLevel::INFO, "线程池创建，工作线程数: " + std::to_string(threads) +
        ", 最大线程数: " + std::to_string(maxThreads));

    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(
            [this, i] { this->workerThread(i); }
        );
    }
}

// 析构函数
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

// 设置最大线程数
void ThreadPool::setMaxThreads(size_t max) {
    std::unique_lock<std::mutex> lock(queue_mutex);

    // 不允许设置小于当前线程数的最大线程数
    if (max < workers.size()) {
        throw std::runtime_error("Cannot set max threads less than current thread count");
    }

    maxThreads = max;
    logger.log(LogLevel::INFO, "设置最大线程数: " + std::to_string(maxThreads));
}

// 获取最大线程数
size_t ThreadPool::getMaxThreads() const {
    return maxThreads;
}

// 获取工作线程数量
size_t ThreadPool::getThreadCount() const {
    return workers.size();
}

// 获取当前活跃的线程数量
size_t ThreadPool::getActiveThreadCount() const {
    return metrics.activeThreads;
}

// 获取当前队列中等待执行的任务数量
size_t ThreadPool::getTaskCount() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    return tasks.size();
}

// 获取已完成的任务数量
size_t ThreadPool::getCompletedTaskCount() const {
    return metrics.completedTasks;
}

// 获取失败的任务数量
size_t ThreadPool::getFailedTaskCount() const {
    return metrics.failedTasks;
}

// 获取当前等待任务的线程数量
size_t ThreadPool::getWaitingThreadCount() const {
    size_t totalThreads = getThreadCount();
    size_t activeCount = getActiveThreadCount();
    return totalThreads - activeCount;
}

// 取消指定ID的任务
bool ThreadPool::cancelTask(const std::string& taskId) {
    std::unique_lock<std::mutex> lock(queue_mutex);

    // 查找任务
    auto it = taskIdMap.find(taskId);
    if (it == taskIdMap.end()) {
        logger.log(LogLevel::ERROR, "尝试取消不存在的任务: " + taskId);
        return false;  // 任务不存在
    }

    auto& taskInfoPtr = it->second;

    // 检查任务状态
    if (taskInfoPtr->status == TaskStatus::RUNNING) {
        logger.log(LogLevel::ERROR, "无法取消正在执行的任务: " + taskId);
        return false;  // 任务已经在执行中，无法取消
    }

    if (taskInfoPtr->status == TaskStatus::COMPLETED ||
        taskInfoPtr->status == TaskStatus::FAILED ||
        taskInfoPtr->status == TaskStatus::CANCELED) {
        logger.log(LogLevel::ERROR, "任务 " + taskId + " 已经处于终止状态: " +
            taskStatusToString(taskInfoPtr->status));
        return false;  // 任务已经完成或失败或已取消
    }

    // 更新任务状态为已取消
    taskInfoPtr->status = TaskStatus::CANCELED;
    logger.log(LogLevel::INFO, "成功取消任务: " + taskId);

    // 注意：我们不从队列中移除任务，而只是更新它的状态
    // 在工作线程中会检查任务状态，如果是已取消，则跳过该任务

    return true;
}

// 动态调整线程池大小
void ThreadPool::resize(size_t threads) {
    std::unique_lock<std::mutex> lock(queue_mutex);

    // 如果线程池已停止，无法调整大小
    if (stop) {
        throw std::runtime_error("resize on stopped ThreadPool");
    }

    // 确保不超过最大线程数
    threads = std::min(threads, maxThreads);

    // 获取当前线程数
    size_t oldSize = workers.size();

    logger.log(LogLevel::INFO, "调整线程池大小: " + std::to_string(oldSize) +
        " -> " + std::to_string(threads) +
        " (最大: " + std::to_string(maxThreads) + ")");

    // 如果新的线程数大于当前线程数，添加新线程
    if (threads > oldSize) {
        workers.reserve(threads);
        for (size_t i = oldSize; i < threads; ++i) {
            workers.emplace_back([this, i] { this->workerThread(i); });
        }
    }
    // 如果新的线程数小于当前线程数，我们需要减少线程
    else if (threads < oldSize) {
        // 清空之前可能存在的待停止线程集合
        threadsToStop.clear();

        // 添加要停止的线程ID到集合
        for (size_t i = threads; i < oldSize; ++i) {
            threadsToStop.insert(i);
        }

        // 解锁并通知
        lock.unlock();
        condition.notify_all();

        // 等待线程结束
        for (size_t i = threads; i < oldSize; ++i) {
            if (workers[i].joinable()) {
                workers[i].join();
            }
        }

        // 重新获取锁和调整容器大小
        lock.lock();

        workers.resize(threads);
    }
}

// 暂停线程池
void ThreadPool::pause() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    paused = true;
    logger.log(LogLevel::INFO, "线程池已暂停");
}

// 恢复线程池
void ThreadPool::resume() {
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

    // 将所有等待中的任务标记为已取消
    for (auto& pair : taskIdMap) {
        if (pair.second->status == TaskStatus::WAITING) {
            pair.second->status = TaskStatus::CANCELED;
        }
    }

    // 仅保留正在执行的任务
    auto it = taskIdMap.begin();
    while (it != taskIdMap.end()) {
        if (it->second->status != TaskStatus::RUNNING) {
            it = taskIdMap.erase(it);
        }
        else {
            ++it;
        }
    }

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

// 获取任务状态
TaskStatus ThreadPool::getTaskStatus(const std::string& taskId) {
    std::unique_lock<std::mutex> lock(queue_mutex);
    auto it = taskIdMap.find(taskId);
    if (it != taskIdMap.end()) {
        return it->second->status;
    }
    return TaskStatus::NOT_FOUND;  // 任务不存在
}

// 获取任务状态字符串
std::string ThreadPool::getTaskStatusString(const std::string& taskId) {
    return taskStatusToString(getTaskStatus(taskId));
}

// 工作线程函数
void ThreadPool::workerThread(size_t id) {
    logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 启动");

    while (true) {
        // 获取任务
        std::shared_ptr<TaskInfo> taskPtr = nullptr;
        TaskFetchResult result = getNextTask(id, taskPtr);

        // 根据获取任务的结果采取不同行动
        switch (result) {
        case TaskFetchResult::SHOULD_EXIT:
            return;  // 线程应该退出

        case TaskFetchResult::NO_TASK:
            continue;  // 没有任务，但应该继续运行，重新等待任务

        case TaskFetchResult::HAS_TASK:
            // 执行任务
            if (taskPtr && taskPtr->task) {
                executeTask(id, taskPtr);
            }
            break;
        }
    }
}

// 获取下一个可执行任务
TaskFetchResult ThreadPool::getNextTask(size_t id, std::shared_ptr<TaskInfo>& taskPtr) {
    std::unique_lock<std::mutex> lock(queue_mutex);

    condition.wait(lock, [this, id] {
        return this->stop ||
            (!this->paused && !this->tasks.empty()) ||
            (this->threadsToStop.find(id) != this->threadsToStop.end());
        });

    // 优先检查线程池是否停止
    if (this->stop) {
        logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 停止（线程池关闭）");
        return TaskFetchResult::SHOULD_EXIT;
    }

    // 检查是否需要终止当前线程
    if (this->threadsToStop.find(id) != this->threadsToStop.end()) {
        this->threadsToStop.erase(id);
        logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 停止（线程池调整大小）");
        return TaskFetchResult::SHOULD_EXIT;
    }

    // 检查是否有任务可执行
    bool foundValidTask = false;

    // 检查是否有任务可执行
    while (!this->paused && !this->tasks.empty()) {
        // 获取队列顶部的任务
        TaskInfo task = this->tasks.top();
        this->tasks.pop();

        // 如果任务有ID，从映射表中查找对应的任务指针
        if (!task.taskId.empty()) {
            auto it = this->taskIdMap.find(task.taskId);
            if (it != this->taskIdMap.end()) {
                taskPtr = it->second;

                // 如果任务已取消，跳过它
                if (taskPtr->status == TaskStatus::CANCELED) {
                    logger.log(LogLevel::DEBUG, "跳过已取消的任务: " + task.taskId);
                    continue;  // 继续检查下一个任务
                }
            }
        }
        else {
            // 对于没有ID的任务，创建一个临时的共享指针
            taskPtr = std::make_shared<TaskInfo>(task);
        }
        // 找到了有效任务
        foundValidTask = true;
        // 记录日志
        std::string taskDesc = taskPtr->taskId.empty() ? "匿名任务" : "任务 " + taskPtr->taskId;
        if (!taskPtr->description.empty()) {
            taskDesc += " (" + taskPtr->description + ")";
        }
        logger.log(LogLevel::DEBUG, "工作线程 " + std::to_string(id) + " 开始执行 " + taskDesc);

        break;  // 跳出while循环
    }

    // 如果找到了有效任务，返回HAS_TASK；否则返回NO_TASK
    return foundValidTask ? TaskFetchResult::HAS_TASK : TaskFetchResult::NO_TASK;
}

// 执行任务，处理正常执行和超时情况
void ThreadPool::executeTask(size_t id, std::shared_ptr<TaskInfo> taskPtr) {
    // 更新任务状态和性能指标
    taskPtr->status = TaskStatus::RUNNING;
    metrics.activeThreads++;
    metrics.updateActiveThreads(metrics.activeThreads);

    auto startTime = std::chrono::steady_clock::now();
    bool isTimeout = false;

    try {
        // 执行任务，处理超时
        if (taskPtr->timeout.count() > 0) {
            executeTaskWithTimeout(taskPtr, isTimeout);
        }
        else {
            // 没有超时限制，直接执行任务，但仍然要处理异常
            taskPtr->task();
        }
        taskPtr->status = TaskStatus::COMPLETED;
        metrics.completedTasks++;
    }
    catch (const std::exception& e) {
        handleTaskException(taskPtr, e.what(), isTimeout);
    }
    catch (...) {
        handleTaskException(taskPtr, "未知异常", false);
    }

    // 计算任务执行时间
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    metrics.addTaskTime(duration.count());

    metrics.activeThreads--;

    // 如果任务有ID，且已完成或失败或超时，从映射表中移除它
    cleanupTask(taskPtr);

    // 记录任务完成日志
    logTaskCompletion(id, taskPtr, duration);
}

#if 1
// 使用超时机制执行任务
void ThreadPool::executeTaskWithTimeout(std::shared_ptr<TaskInfo> taskPtr, bool& isTimeout) {
    // 创建一个信号量，用于指示任务是否完成
    std::atomic<bool> taskCompleted{ false };
    std::exception_ptr taskException = nullptr;

    // 在单独的线程中执行任务
    std::thread taskThread([&]() {
        try {
            taskPtr->task();
            taskCompleted = true;
        }
        catch (...) {
            taskException = std::current_exception();
            taskCompleted = true;
        }
        });

    // 分离线程，允许它在后台运行
    taskThread.detach();

    // 等待任务完成或超时
    auto startTime = std::chrono::steady_clock::now();
    auto waitUntil = startTime + taskPtr->timeout;
    while (std::chrono::steady_clock::now() < waitUntil && !taskCompleted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 如果任务未完成，说明超时
    if (!taskCompleted) {
        isTimeout = true;
        throw std::runtime_error("Task timed out after " +
            std::to_string(taskPtr->timeout.count()) + "ms");
    }

    // 如果任务抛出了异常，重新抛出
    if (taskException) {
        std::rethrow_exception(taskException);
    }
}
#endif

// 处理任务异常
void ThreadPool::handleTaskException(std::shared_ptr<TaskInfo> taskPtr, const std::string& errorMessage, bool isTimeout) {
    taskPtr->status = TaskStatus::FAILED;
    taskPtr->errorMessage = errorMessage;

    if (isTimeout) {
        metrics.timedOutTasks++;
        logger.log(LogLevel::ERROR, "任务超时: " + errorMessage);
    }
    else {
        metrics.failedTasks++;
        logger.log(LogLevel::ERROR, "任务异常: " + errorMessage);
    }
}

// 清理任务
void ThreadPool::cleanupTask(std::shared_ptr<TaskInfo> taskPtr) {
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (!taskPtr->taskId.empty()) {
        taskIdMap.erase(taskPtr->taskId);
    }
    waitCondition.notify_all();
}

// 记录任务完成日志
void ThreadPool::logTaskCompletion(size_t id, std::shared_ptr<TaskInfo> taskPtr, const std::chrono::nanoseconds& duration) {
    std::string taskDesc = taskPtr->taskId.empty() ? "匿名任务" : "任务 " + taskPtr->taskId;
    std::string statusStr = taskStatusToString(taskPtr->status);

    logger.log(LogLevel::DEBUG,
        "工作线程 " + std::to_string(id) + " " + statusStr + " " + taskDesc +
        " (用时: " + std::to_string(duration.count() / 1000000.0) + "ms)");
}

// 记录任务提交日志
void ThreadPool::logTaskSubmission(const std::string& taskId, const std::string& description,
    TaskPriority priority, std::chrono::milliseconds timeout) {
    std::string priorityStr = priorityToString(priority);

    if (!taskId.empty() || !description.empty()) {
        std::string timeoutStr = (timeout.count() > 0) ?
            std::to_string(timeout.count()) + "ms" : "无";
        logger.log(LogLevel::DEBUG, "提交任务 " + taskId + " (" + description +
            ") 优先级: " + priorityStr + ", 超时: " + timeoutStr);
    }
    else {
        logger.log(LogLevel::DEBUG, "提交" + priorityStr + "优先级任务");
    }
}

// 检查并创建任务信息,并提交到队列
std::shared_ptr<TaskInfo> ThreadPool::checkAndCreateTaskInfo(
    const std::string& taskId,
    const std::string& description,
    TaskPriority priority,
    std::chrono::milliseconds timeout,
    std::function<void()> taskFunction) {

    std::unique_lock<std::mutex> lock(queue_mutex);

    // 线程池已停止，不能添加任务
    if (stop) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
    }

    // 检查任务ID是否已存在
    if (!taskId.empty() && taskIdMap.find(taskId) != taskIdMap.end()) {
        throw std::runtime_error("Task ID '" + taskId + "' already exists");
    }

    // 记录日志
    logTaskSubmission(taskId, description, priority, timeout);

    // 创建任务信息的共享指针
    auto taskInfoPtr = std::make_shared<TaskInfo>(
        std::move(taskFunction),
        priority,
        taskId,
        description,
        timeout
    );

    // 如果有任务ID，添加到映射表
    if (!taskId.empty()) {
        taskIdMap[taskId] = taskInfoPtr;
    }

    // 添加任务到优先级队列
    tasks.push(*taskInfoPtr);

    // 更新性能指标
    metrics.totalTasks++;
    metrics.updateQueueSize(tasks.size());

    return taskInfoPtr;
}
