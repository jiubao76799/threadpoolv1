#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include "ThreadPool.h"

//测试函数:耗时计算
int longComputation( int id ,int duration){
    std::cout<<"Task"<<id<<"started,duration:"<<duration<<"ms"<<std::endl;
    std::this_thread ::sleep_for(std::chrono::milliseconds(duration));
    std::cout<<"Task"<<id<<"completed"<<std::endl;
    return duration;
}

//测试函数:可能抛出异常的任务
int errorProneTask(int id, bool shouldFail){
    std::cout<<"ErrorProneTask"<<id<<"started"<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if(shouldFail){
        std::cout<<"ErrorProneTask"<<id<<"throwing exception"<<std::endl;
        throw std::runtime_error("Task failed on purpose");
    }

    std::cout<<"ErrorProneTask"<<id<<"completed successfully"<<std::endl;
    return id;
}

//打印线程池状态的辅助函数
void printPoolStatus(ThreadPool& pool, const std::string& stage) {
    std::cout << "\n=== " << stage << " ===" << std::endl;
    std::cout << "  线程数: " << pool.getThreadCount() << std::endl;
    std::cout << "  活跃线程数: " << pool.getActiveThreadCount() << std::endl;
    std::cout << "  等待线程数: " << pool.getWaitingThreadCount() << std::endl;
    std::cout << "  等待任务数: " << pool.getTaskCount() << std::endl;
    std::cout << "  已完成任务数: " << pool.getCompletedTaskCount() << std::endl;
    std::cout << "  失败任务数: " << pool.getFailedTaskCount() << std::endl;
}

int main() {
    std::cout<<"===第五天测试==="<<std::endl;

    try {
        //创建线程池，线程数等于硬件并发数
        size_t threadCount = std::thread::hardware_concurrency();
        threadCount = threadCount == 0 ? 1:threadCount;

        //为了更好地观察效果，我们使用较少的线程数
         size_t poolThreads = std::min(threadCount, (size_t)4);
        
        std::cout << "系统有 " << threadCount << " 个CPU核心" << std::endl;
        std::cout << "创建拥有 " << poolThreads << " 个线程的线程池" << std::endl;
        
        ThreadPool pool(poolThreads);

        //显示初始状态
        printPoolStatus(pool,"初始状态");

        //随机数生成器
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> durDist(100,300);//100-300ms之间

        //提交一些任务
        std::cout<<"\n提交6个正常任务..."<<std::endl;
        std::vector<std::future<int>> results;
        for(int i = 0; i < 6; ++i){
            int duration = durDist(gen);
            results.push_back(pool.enqueue(longComputation,i,duration));
        }
        
        //显示任务提交后的状态
        //printPoolStatus(pool,"任务提交后的状态");

        //等待一段时间,让一些任务完成
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        //显示部分任务完成后的状态
        printPoolStatus(pool,"部分任务完成后的状态");

        //测试暂停功能
        std::cout<<"\n---测试暂停/恢复功能---"<<std::endl;
        pool.pause();

        //提交更多任务（这些任务会被暂停）
        std::cout<<"线程池暂停后，提交3个任务..."<<std::endl;
        for(int i = 10; i< 13; ++i){
            int duration = durDist(gen);
            results.push_back(pool.enqueue(longComputation,i,duration));
        }

        //显示暂停后的状态
        printPoolStatus(pool,"暂停后的状态");

        //等待一段时间
        std::cout<<"等待1秒..."<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        //显示等待后的状态(应该没什么变化，因为被暂停了)
        printPoolStatus(pool,"等待后的状态(暂停中)");

        //恢复线程池
        pool.resume();

        //等待一段时间，让更多任务完成
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        //显示恢复后的状态
        printPoolStatus(pool,"恢复后的状态");

        //测试动态调整线程数量
        std::cout<<"\n---测试动态调整线程数量---"<<std::endl;
        size_t newThreadCount = poolThreads + 2;
        std::cout<<"增加线程数到"<<newThreadCount<<"..."<<std::endl;
        pool.resize(newThreadCount);

        //显示增加线程后的状态
        printPoolStatus(pool,"增加线程后的状态");

        //测试减少线程数量
        newThreadCount = poolThreads ;
        std::cout<<"减少线程数到"<<newThreadCount<<"..."<<std::endl;
        pool.resize(newThreadCount);

        //显示减少线程后的状态
        printPoolStatus(pool,"减少线程后的状态");

        //测试异常处理
        std::cout<<"\n---测试异常处理---"<<std::endl;
        std::vector<std::future<int>> errorResults;
        for(int i = 0; i < 6; ++i){
            bool shouldFail = (i % 3 == 0);
            errorResults.push_back(pool.enqueue(errorProneTask,i,shouldFail));
        }

        //等待并获取可能抛出异常的任务的结果
        std::cout<<"\n等待异常任务完成..."<<std::endl;
        for(size_t i = 0; i < errorResults.size(); ++i){
            try{
                int result = errorResults[i].get();
                std::cout<<"Error-prone Task"<<i<<"succeeded with result:"<<result<<std::endl;
            }catch(const std::exception& e){
                std::cout<<"Error-prone Task"<<i<<"failed:"<<e.what()<<std::endl;
            } 
        }

        //测试清空任务队列
        std::cout<<"\n---测试清空任务队列---"<<std::endl;
        for(int i = 100; i< 105; ++i){
            int duration = durDist(gen);
            pool.enqueue(longComputation,i,duration);
        }

        //显示提交任务后的状态
        printPoolStatus(pool,"提交清空测试任务后的状态");

        //清空任务队列
        pool.clearTasks();

        //显示清空队列后的状态
        printPoolStatus(pool,"清空任务队列后的状态");

        //测试等待所有任务完成
        std::cout<<"\n---测试等待所有任务完成---"<<std::endl;

        //等待所有正常任务的结果
        std::cout<<"\n等待正常任务完成..."<<std::endl;
        for(size_t i = 0; i < results.size(); ++i){
            try{
                int duration = results[i].get();
                std::cout<<"Normal task"<<i<<"result:"<<duration<<"ms"<<std::endl;  
            }catch(const std::exception& e){
                std::cout<<"Normal task"<<i<<"failed:"<<e.what()<<std::endl;
            }
        }

        //等待所有任务完成
        pool.waitForTasks();

        //显示最终状态
        printPoolStatus(pool,"最终状态");

        std::cout<<"\n---验证线程池控制功能---"<<std::endl;
        std::cout<<"所有任务处理完成，线程池控制功能正常"<<std::endl;
        std::cout<<"线程池是否已停止:"<<(pool.isStopped() ? "是":"否")<<std::endl;

    }catch(const std::exception& e){
        std::cerr<<"发生异常:"<<e.what()<<std::endl;
        return 1;
    }

    std::cout<<"\n===第五天测试完成==="<<std::endl;
    std::cout<<"线程池控制功能(resize pause/resume waitForTasks clearTasks)正常!"<<std::endl;

    return 0;
}