#include <iostream>
#include <thread>
#include "ThreadPool.h"

int main() {
    
    
    std::cout << "创建线程池..."<< std::endl;

    try {
        //创建一个拥有四个工作线程的线程池
        ThreadPool pool(4);

        std::cout << "线程池创建成功!" << std::endl;
        std::cout << "线程池中有4个工作线程"<< std::endl;

        //让程序稍作停留， 以便观察输出
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        //此时我们还没有实现具体功能，所以线程池会立即被销毁
    } catch (const std::exception& e){
        std::cerr << "发生异常：" << e.what() << std::endl;
        return 1;
    }

    std::cout <<"主函数结束，线程池已被销毁" << std::endl;

    return 0;
    
}