#include "ThreadPool.h"
#include <iostream>

//构造函数 - 创建指定数量的工作线程，每个线程运行workerThread函数
ThreadPool::ThreadPool(size_t threads) {
    std::cout<<"线程池构造函数被调用，创建"<< threads <<"个线程"<<std::endl;

    for(size_t i = 0; i < threads; ++i ){
        //创建线程并传入线程内容 ，此处为工作线程函数
        workers.emplace_back(
            [this, i]{this->workerThread(i);}
        );

    }

    std::cout<<"所有工作线程创建完成"<<std::endl;
}

//析构函数 优雅地关闭线程池
ThreadPool::~ThreadPool() {
    std::cout<<"线程池开始关闭"<<std::endl;

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all();

    for(std::thread &worker : workers){
        if(worker.joinable()){
            worker.join();
        }
    }

    std::cout<<"线程池已关闭"<<std::endl;
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
    return completedTasks;
}


//获取当前活跃的线程数量
size_t ThreadPool::getActiveThreadCount() const{
    return activeThreads;
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
    return failedTasks;
}

//动态调整线程池大小
void ThreadPool::resize(size_t threads){
    std::unique_lock<std::mutex> lock(queue_mutex);

    if(stop){
        throw std::runtime_error("resize on stopped ThreadPool");
    }

    //获取当前线程数
    size_t oldSize = workers.size();

    std::cout<<"调整线程大小:"<<oldSize<<"->"<<threads<<std::endl;

    //如果新的线程数大于当前线程数，添加新线程
    if(threads > oldSize){
        workers.reserve(threads);
        for(size_t i = oldSize; i < threads; ++i){
            workers.emplace_back([this,i]{this->workerThread(i);});
        }
        std::cout<<"增加了"<<(threads - oldSize)<<"个工作线程"<<std::endl;
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
        std::cout<<"减少了"<<(oldSize - threads)<<"个工作线程"<<std::endl;
    }

}

//暂停线程池
void ThreadPool::pause(){
    std::unique_lock<std::mutex> lock(queue_mutex);
    paused = true;
    std::cout<<"线程池已暂停"<<std::endl;
}

//恢复线程池
void ThreadPool::resume(){
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        paused = false;
        std::cout<<"线程池已恢复"<<std::endl;
    }
    condition.notify_all();
}

//等待所有任务完成
void ThreadPool::waitForTasks(){
    std::unique_lock<std::mutex> lock(queue_mutex);
    std::cout<<"等待所有任务完成..."<<std::endl;
    waitCondition.wait(lock,[this]{
        return (tasks.empty() && activeThreads == 0)|| stop;
    });
    std::cout<<"所有任务已完成"<<std::endl;
}

//清空任务队列
void ThreadPool::clearTasks(){
    std::unique_lock<std::mutex> lock(queue_mutex);
    size_t taskCount = tasks.size();

    std::queue<std::function<void()>> emptyQueue;
    std::swap(tasks, emptyQueue);

    std::cout<<"清空任务队列:"<<taskCount<<"个任务被移除"<<std::endl;
}

//工作线程函数 - 线程的工作逻辑
void ThreadPool::workerThread(size_t id){
    while(true){
        std::function<void()> task;

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
                return;
            }

            //检查是否要终止当前线程
            if(this->threadsToStop.find(id) != this->threadsToStop.end()){
                this->threadsToStop.erase(id);
                return;
            }

            //最后检查是否有任务可执行
            if(!this->paused &&!this->tasks.empty()){
                task = std::move(this->tasks.front());
                this->tasks.pop();
            }  
        }

        //执行任务并处理异常 (在锁外执行，避免长时间持有锁)
        if(task){
            ++activeThreads;
            try {
                task();
                ++completedTasks;
            }catch (const std::exception& e){
                std::cerr<<"异常发生在任务中:"<<e.what()<<std::endl;
                ++failedTasks;
            }catch(...){
                std::cerr<<"未知异常发生在任务中"<<std::endl;
                ++failedTasks;
            }
            --activeThreads;
            waitCondition.notify_all();
        }
    }
}

