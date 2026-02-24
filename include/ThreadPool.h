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

class ThreadPool {
public: 
    //构造函数，创建指定数量的线程
    ThreadPool(size_t threads);

    //禁用拷贝构造函数和赋值运算符
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    //析构函数
    ~ThreadPool();

    //提交任务到线程池
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
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



    //状态查询方法
    bool isStopped() const { return stop; }
    
private:
    //工作线程函数
    void workerThread(size_t id);

    //存储需要停止的线程ID(索引)
    std::unordered_set<size_t> threadsToStop;
    
    //工作线程容器
    std::vector<std::thread> workers;

    //任务队列
    std::queue<std::function<void()>> tasks;

    //同步机制
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable waitCondition;

    //控制线程池状态
    std::atomic<bool> stop{false};
    std::atomic<bool> paused{false};

    //计数器
    std::atomic<size_t> activeThreads{0};
    std::atomic<size_t> completedTasks{0};
    std::atomic<size_t> failedTasks{0};
    
};

//模板函数实现
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f,Args&&... args)
    ->std::future<typename std::result_of<F(Args...)>::type>{

        using return_type= typename std::result_of<F(Args...)>::type;
        
        //创建任务包装
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        );

       std::future<return_type> result = task->get_future();

       {
            std::unique_lock<std::mutex> lock(queue_mutex);

            //线程池已停止，不能添加任务，抛出异常
            if(stop){
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            //添加任务到队列
            tasks.emplace([task](){ (*task)(); });
       }

       condition.notify_one(); //通知一个工作线程有新任务
       return result;
    }




#endif // THREAD_POOL_H
