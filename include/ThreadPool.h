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

class ThreadPool {
public: 
    //构造函数，创建指定数量的线程
    ThreadPool(size_t threads);

    //禁用拷贝构造函数和赋值运算符
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    //析构函数
    ~ThreadPool();

private:
    //工作线程容器
    std::vector<std::thread> workers;

    //任务队列
    std::queue<std::function<void()>> tasks;

    //同步机制
    std::mutex queue_mutex;
    std::condition_variable condition;

    //控制线程池停止
    std::atomic<bool> stop{false};

};





#endif // THREAD_POOL_H
