#include <iostream>
#include <chrono>
#include <thread>
#include "ThreadPool.h"

void sleepFor(int seconds) {
    std::cout<<"开始睡眠"<<seconds<<"秒..."<<std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    std::cout<<"睡眠"<<seconds<<"秒结束"<<std::endl;

}

int main() {
    std::cout<<"=== 第二天测试 ===" <<std::endl;
    std::cout<<"创建线程池..."<<std::endl;

    //获取cpu核心数
    size_t threadCount = std::thread::hardware_concurrency();
    std::cout<<"系统有"<<threadCount<<"个CPU核心"<<std::endl;

    //确保有一个线程
    threadCount = threadCount == 0 ? 1 : threadCount;

    //为了更好地观察效果，我们使用较少的线程数
    size_t poolThreads = std::min(threadCount,(size_t)4);

    try{
        std::cout<<"\n---测试线程池创建---"<<std::endl;
        //创建线程池
        ThreadPool pool(poolThreads);

        std::cout<<"线程池创建成功！"<<std::endl;
        std::cout<<"线程池中有"<<poolThreads<<"个工作线程"<<std::endl;

        //测试线程池状态
        //std::cout<<"\n---测试线程池状态---"<<std::endl;
        //std::cout<<"线程池是否已停止"<<(pool.isStopped() ? "是" : "否")<<std::endl;   

        //让主线程等待一段时间，观察工作线程的行为
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::cout<<"\n---准备销毁线程池---"<<std::endl;
        std::cout<<"注意：第三天我们将实现任务提交功能，但还不能提交任务"<<std::endl;
        std::cout<<"目前线程池只能创建工作线程，但还不能提交任务"<<std::endl;

        //线程池会在离开作用域时自动销毁，触发析构函数
    }catch(const std::exception& e){
        std::cerr<<"发生异常："<<e.what()<<std::endl;
        return 1;
    }

    std::cout<<"\n---测试完成---"<<std::endl;
    std::cout<<"主函数结束，线程池已被销毁"<<std::endl;

    return 0;
}