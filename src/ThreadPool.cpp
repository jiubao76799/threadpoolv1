#include "ThreadPool.h"
#include <iostream>

//构造函数 - 第一天只是简单的占位实现
ThreadPool::ThreadPool(size_t threads) {
    std::cout << "线程池构造函数被调用，线程数：" << threads <<std::endl ;
    // 注意第一天我们还没有实现具体的线程创建逻辑
    //这里只是一个占位符，让程序能够编译和运行
}


ThreadPool::~ThreadPool() {
    std::cout << "线程池析构函数被调用" << std::endl;
    // 依旧占位符
}
