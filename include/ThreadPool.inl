#ifndef THREAD_POOL_INL
#define THREAD_POOL_INL

// 默认任务提交（中等优先级）
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
    return enqueueWithPriority(TaskPriority::MEDIUM,
        std::forward<F>(f),
        std::forward<Args>(args)...);
}

// 创建任务函数
template<class F, class... Args>
auto ThreadPool::createTaskFunction(std::shared_ptr<std::promise<typename std::result_of<F(Args...)>::type>> promise,
    F&& f, Args&&... args)
    -> std::function<void()> {

    using return_type = typename std::invoke_result<F, Args...>::type;

    return [promise,
        f = std::forward<F>(f),
        args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        try {
            // 对于void返回类型和非void返回类型分别处理
            if constexpr (std::is_void_v<return_type>) {
                std::apply(f, args);
                promise->set_value();
            }
            else {
                promise->set_value(std::apply(f, args));
            }
        }
        catch (const std::exception&) {
            // 设置异常到promise，让future可以获取到异常
            promise->set_exception(std::current_exception());
            throw;
        }
        catch (...) {
            // 处理其他类型的异常
            promise->set_exception(std::current_exception());
            throw;
        }
    };
}

// 带优先级的任务提交
template<class F, class... Args>
auto ThreadPool::enqueueWithPriority(TaskPriority priority, F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {

     // 使用空的ID和描述，复用enqueueWithInfo实现
    return enqueueWithInfo("", "", priority,
        std::forward<F>(f), std::forward<Args>(args)...);
}

// 带ID和描述的任务提交
template<class F, class... Args>
auto ThreadPool::enqueueWithInfo(std::string taskId, std::string description,
    TaskPriority priority, F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {

    using return_type = typename std::invoke_result<F, Args...>::type;

    // 创建promise - 在锁之外
    auto promise = std::make_shared<std::promise<return_type>>();
    std::future<return_type> result = promise->get_future();

    // 创建任务函数 - 在锁之外
    auto taskFunction = createTaskFunction(promise, std::forward<F>(f), std::forward<Args>(args)...);

    // 仅在需要修改共享数据时加锁
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // 线程池已停止，不能添加任务
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        // 检查任务ID是否已存在  下节课支持
       // if (!taskId.empty() && taskIdMap.find(taskId) != taskIdMap.end()) {
     //       throw std::runtime_error("Task ID '" + taskId + "' already exists");
       // }

        // 记录日志
        logTaskSubmission(taskId, description, priority);

        // 添加任务到优先级队列
        tasks.emplace(
            std::move(taskFunction),
            priority,
            taskId,
            description
        );

        // 如果有任务ID，添加到映射表
        if (!taskId.empty()) {
            taskIdMap[taskId] = tasks.size();
        }

        // 更新性能指标
        metrics.totalTasks++;
        metrics.updateQueueSize(tasks.size());
    }

    condition.notify_one();
    return result;
}

#endif // THREAD_POOL_INL