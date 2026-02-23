#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include "ThreadPool.h"

//测试函数:耗时计算
int longComputation(int duration){
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
    return duration;
}

//测试函数:模拟可能抛出异常的任务
int errorProneTask(int n){
    if(n % 3 == 0){
        throw std::runtime_error("Number is divisible by 3!");
    }
    return n*n;
}

//打印线程池状态的辅助函数
void printPoolStatus(ThreadPool& pool,const std::string& stage){
    std::cout<<"\n=== "<<stage<<" ==="<<std::endl;
    std::cout<<"总线程数: "<<pool.getThreadCount()<<std::endl;
    std::cout<<"活跃线程数: "<<pool.getActiveThreadCount()<<std::endl;
    std::cout<<"等待线程数:"<<pool.getWaitingThreadCount()<<std::endl;
    std::cout<<"等待任务数: "<<pool.getTaskCount()<<std::endl;
    std::cout<<"已完成任务数: "<<pool.getCompletedTaskCount()<<std::endl;
}

int main() {
    std::cout<<"===第四天测试==="<<std::endl;

    try{
        //创建线程池，线程数等于硬件并发数
        size_t threadCount = std::thread::hardware_concurrency();
        threadCount = threadCount == 0 ? 1 : threadCount;

        //为了更好观察效果，我们使用较少的线程数
        size_t poolThreads = std::min(threadCount,(size_t)4);

        std::cout<<"系统有"<<threadCount<<"个cpu核心"<<std::endl;
        std::cout<<"创建拥有"<<poolThreads<<"个线程的线程池"<<std::endl;

        ThreadPool pool(poolThreads);

        //显示初始状态
        printPoolStatus(pool,"初始状态");

        //随机数生成器，用于生成任务的持续时间
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> durDist(100,500); //任务持续时间在100-500毫秒之间

        //提交正常任务
        std::cout<<"\n---提交10个正常任务---"<<std::endl;
        std::vector<std::future<int>>  results;
        for(int i = 0; i < 10 ; ++i){
            int duration = durDist(gen);
            results.push_back(pool.enqueue(longComputation,duration));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));//等待50ms
        printPoolStatus(pool,"提交正常任务后（延迟观察）");

        //提交可能抛出异常的任务
        std::cout<<"\n---提交10个可能抛出异常的任务---"<<std::endl;
        std::vector<std::future<int>> errorResults;
        for(int i = 0; i < 10 ; ++i){
            errorResults.push_back(pool.enqueue(errorProneTask,i));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));//等待50ms
        printPoolStatus(pool,"提交异常任务后");

        //等待并获取所有正常任务的结果
        std::cout<<"\n---等待正常任务完成..."<<std::endl;
        for(size_t i = 0; i < results.size(); ++i){
            try{
                int duration = results[i].get();
                std::cout<<"正常任务"<<i<<"完成，耗时"<<duration<<"ms"<<std::endl;
            }catch (const std::exception& e){
                std::cout<<"正常任务"<<i<<"抛出异常:"<<e.what()<<std::endl;
            }
        }

        //显示最终状态
        printPoolStatus(pool,"最终状态");

        std::cout<<"\n---验证异常处理---"<<std::endl;
        std::cout<<"所有任务处理完成，线程池人在正常运行"<<std::endl;
        std::cout<<"线程池是否已停止:"<<(pool.isStopped() ? "是" : "否")<<std::endl;

        //测试原子操作的正确性
        std::cout<<"\n---原子操作验证---"<<std::endl;
        std::cout<<"活跃线程数应该为0:"<<pool.getActiveThreadCount()<<std::endl;
        std::cout<<"等待线程数数应该为"<<poolThreads<<":"<<pool.getWaitingThreadCount()<<std::endl;
        std::cout<<"已完成任务数应该为20:"<<pool.getCompletedTaskCount()<<std::endl;

        //验证线程数量关系
        std::cout<<"\n---线程数量关系验证---"<<std::endl;
        std::cout<<"总线程数 = 总活跃线程数 + 等待线程数"<<std::endl;
        std::cout<<pool.getThreadCount()<<"="<<pool.getActiveThreadCount()<<"+"<<pool.getWaitingThreadCount()<<std::endl;

        size_t sum=pool.getActiveThreadCount() + pool.getWaitingThreadCount();
        if(sum  == pool.getThreadCount()){
            std::cout<<"线程数量关系正确"<<std::endl;
        }else{
            std::cout<<"线程数量关系错误"<<std::endl;
        }
    }catch (const std::exception& e){
        std::cerr<<"发生异常: "<<e.what()<<std::endl;
        return 1;
    }

    std::cout << "\n=== 第四天测试完成 ===" << std::endl;
    std::cout << "线程池状态管理和异常处理功能正常！" << std::endl;
    
    return 0;
}