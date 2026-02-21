#include "ThreadPool.h"
#include <iostream>

//构造函数 - 创建指定数量的工作线程
ThreadPool::ThreadPool(size_t threads) {
    std::cout<<"线程池构造函数被调用，创建"<< threads <<"个线程"<<std::endl;

    for(size_t i = 0; i < threads; ++i ){
        //创建线程并传入线程内容 ，此处为工作线程函数
        workers.emplace_back(
            [this]{this->workerThread();}
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

//工作线程函数 - 线程的工作逻辑
void ThreadPool::workerThread(){
    while(true){
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            std::cout<< "工作线程" <<std::this_thread::get_id()<<"等待任务或停止信号..."<<std::endl;
            //等待直到有任务或者线程池停止
            condition.wait(lock, [this]{
                return this->stop || !this->tasks.empty();
            });

            //唤醒后的第一件事是检查是否应该退出，如果线程池停止且任务队列为空，退出线程
            if(this->stop && this->tasks.empty()){
                return;
            }

            //获取任务
            if(!this->tasks.empty()){
                task = std::move(this->tasks.front());
                this->tasks.pop();
            }  
        }

        //执行任务 (在锁外执行，避免长时间持有锁)
        if(task){
            try {
                task();
            }catch (const std::exception& e){
                std::cout << "任务执行异常:"<<e.what() <<std::endl;
            }catch(...){
                std::cout<<"任务执行未知异常"<<std::endl;
            }
        }
    }
}

