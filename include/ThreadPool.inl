#ifndef THREAD_POOL_INL
#define THREAD_POOL_INL

// 默认任务提交（中等优先级，可选超时参数）
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
    return enqueueWithPriority(TaskPriority::MEDIUM,
        std::chrono::milliseconds(0),
        std::forward<F>(f),
        std::forward<Args>(args)...);
}

// 带优先级的任务提交（可选超时参数）
template<class F, class... Args>
auto ThreadPool::enqueueWithPriority(TaskPriority priority, std::chrono::milliseconds timeout,
    F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {

    // 使用空的ID和描述，复用enqueueWithInfo实现
    return enqueueWithInfo("", "", priority, timeout,
        std::forward<F>(f), std::forward<Args>(args)...);
}

// 批量提交任务（可选超时参数）
template<class F>
std::vector<std::future<void>> ThreadPool::enqueueMany(const std::vector<F>& tasks,
    TaskPriority priority, std::chrono::milliseconds timeout) {
    std::vector<std::future<void>> futures;
    futures.reserve(tasks.size());

    for (const auto& task : tasks) {
        // 对于每个已经绑定了所有参数的任务对象，直接提交
        futures.push_back(enqueueWithInfo("", "", priority, timeout, task));
    }

    return futures;  // 返回future集合，允许调用者等待任务完成
}

// 带任务ID前缀的批量提交任务（可选超时参数）
template<class F>
std::vector<std::future<void>> ThreadPool::enqueueManyWithIdPrefix(const std::string& idPrefix,
    const std::string& descriptionPrefix,
    const std::vector<F>& tasks,
    TaskPriority priority, std::chrono::milliseconds timeout) {

    std::vector<std::future<void>> futures;
    futures.reserve(tasks.size());

    for (size_t i = 0; i < tasks.size(); ++i) {
        std::string taskId = idPrefix + "-" + std::to_string(i);
        std::string description = descriptionPrefix + " " + std::to_string(i);
        futures.push_back(enqueueWithInfo(taskId, description, priority, timeout, tasks[i]));
    }

    return futures;  // 返回future集合，允许调用者等待任务完成
}
#if 1
// 创建任务函数
template<class F, class... Args>
auto ThreadPool::createTaskFunction(std::shared_ptr<std::promise<typename std::result_of<F(Args...)>::type>> promise,
    F&& f, Args&&... args)
    -> std::function<void()> {

    using return_type = typename std::result_of<F(Args...)>::type;

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
            throw;  // 重新抛出异常让executeTask处理
        }
        catch (...) {
            // 处理其他类型的异常
            promise->set_exception(std::current_exception());
            throw; 
        }
    };
}

#endif

// 带ID、描述和可选超时的任务提交实现（主函数）
template<class F, class... Args>
auto ThreadPool::enqueueWithInfo(std::string taskId, std::string description,
    TaskPriority priority,
    std::chrono::milliseconds timeout,
    F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {

    using return_type = typename std::result_of<F(Args...)>::type;

    // 创建promise - 在锁之外
    auto promise = std::make_shared<std::promise<return_type>>();
    std::future<return_type> result = promise->get_future();

    // 创建任务函数 - 在锁之外
    auto taskFunction = createTaskFunction(promise, std::forward<F>(f), std::forward<Args>(args)...);

    // 检查并创建任务信息 - 这个函数内部会加锁
    checkAndCreateTaskInfo(taskId, description, priority, timeout, std::move(taskFunction));

    // 通知等待的线程
    condition.notify_one();
    return result;
}

#endif // THREAD_POOL_INL
