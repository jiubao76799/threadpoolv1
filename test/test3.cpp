#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <thread>
#include "ThreadPool.h"

//测试函数：计算斐波那契数
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

//测试函数：返回字符串
std::string getMessage(const std::string& name, int waitSeconds){
    std::this_thread::sleep_for(std::chrono::seconds(waitSeconds));
    return "Hello, " + name + "! (waited " + std::to_string(waitSeconds)  + "s)";
}

//测试函数：无返回值
void printMessage(const std::string& message){
    std::cout<<"Message:"<<message<<std::endl;
}

int main() {
    std::cout<<"=== 第三天测试 ==="<<std::endl;

    try {
        //创建线程池，线程数等于硬件并发数
        size_t threadCount = std::thread::hardware_concurrency();
        threadCount = threadCount == 0  ? 1 :threadCount;

        //为了更好观察效果，我们使用较少的线程数
        size_t poolThreads = std::min(threadCount,(size_t)4);

        std::cout<<"系统有"<<threadCount<<"个cpu核心"<<std::endl;
        std::cout<<"创建拥有"<<poolThreads<<"个线程的线程池"<<std::endl;

        ThreadPool pool(poolThreads);

        //存储future对象
        std::vector<std::future<int>> fibs;
        std::vector<std::future<std::string>> msgs;
        std::vector<std::future<void>> prints;

        std::cout<<"\n---提交不同类型的任务---"<<std::endl;

        std::cout<<"提交斐波那契计算任务..."<<std::endl;
        //提交斐波那契计算任务(较少数字，避免计算时间过长) 此处的enqueue会返回一个future对象，以便后续获取结果
        for (int i = 20; i < 25 ;i++) {
            fibs.push_back(
                pool.enqueue(fibonacci,i)
            );
        }

        std::cout<<"提交获取消息任务..."<<std::endl;
        //提交获取消息的任务
        for(int i = 1; i <= 3; ++i){
            msgs.push_back(
                pool.enqueue(getMessage,"User"+std::to_string(i),1)
            );
        }

        std::cout<<"提交打印消息任务..."<<std::endl;
        //提交打印消息的任务
        for(int i = 0; i < 3; ++i){
            prints.push_back(
                pool.enqueue(printMessage,"This is a message "+ std::to_string(i))
            );
        }

        std::cout<<"\n---获取任务结果---"<<std::endl;

        //获取斐波那契结果
        std::cout << "斐波那契结果:" << std::endl;
        for (size_t i = 0; i < fibs.size(); ++i){
            std::cout<<"fibonacci(" << (i + 20) << ") = " << fibs[i].get() << std::endl;    
        }

        //获取消息结果
        std::cout << "\n消息结果:" << std::endl;
        for(auto& future : msgs){
            std::cout<<future.get()<<std::endl; 
        }

        //等待打印任务完成
        std::cout<<"\n等待打印任务完成..."<<std::endl;
        for (auto& future : prints){
            future.wait();
        }

        std::cout<<"\n---测试完成---"<<std::endl;
        std::cout<<"所有任务完成!线程池功能正常"<<std::endl;

        //测试线程池状态
        std::cout<<"线程池是否已停止: "<<(pool.isStopped() ? "是" : "否")<<std::endl;

    } catch (const std::exception& e){
        std::cerr<<"发生异常: "<<e.what()<<std::endl;
        return 1;
    }
    return 0;
    
}
