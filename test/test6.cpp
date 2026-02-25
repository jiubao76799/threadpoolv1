#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>
#include <thread>
#include <cmath>
#include "ThreadPool.h"

// 获取优先级字符串
std::string getPriorityString(TaskPriority priority) {
    switch (priority) {
        case TaskPriority::LOW:      return "低优先级";
        case TaskPriority::MEDIUM:   return "中优先级";
        case TaskPriority::HIGH:     return "高优先级";
        case TaskPriority::CRITICAL: return "关键优先级";
        default:                     return "未知优先级";
    }
}

// 简化的计算任务 - 只用于测试优先级
int simpleComputeTask(int id, TaskPriority priority) {
    std::cout << "[执行] " << getPriorityString(priority) << " 任务 " << id << std::endl;
    
    // 模拟少量计算工作
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return id;
}

// 模拟IO任务
std::string ioTask(const std::string& name, int delay, TaskPriority priority) {
    std::cout << "[开始] " << getPriorityString(priority) << " IO任务 " << name << " 开始，模拟延迟: " << delay << "ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    std::cout << "[完成] " << getPriorityString(priority) << " IO任务 " << name << " 完成" << std::endl;
    return "IO结果: " + name;
}

// 模拟可能失败的任务
bool riskyTask(int id, bool shouldFail, TaskPriority priority) {
    std::cout << "[开始] " << getPriorityString(priority) << " 风险任务 " << id << " 开始" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (shouldFail) {
        std::cout << "[失败] " << getPriorityString(priority) << " 风险任务 " << id << " 即将失败" << std::endl;
        throw std::runtime_error("任务 " + std::to_string(id) + " 故意失败");
    }

    std::cout << "[完成] " << getPriorityString(priority) << " 风险任务 " << id << " 成功完成" << std::endl;
    return true;
}

// 打印分隔线
void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

// 打印线程池状态
void printPoolStatus(ThreadPool& pool, const std::string& stage) {
    std::cout << "\n--- " << stage << " ---" << std::endl;
    std::cout << "线程数: " << pool.getThreadCount() << std::endl;
    std::cout << "活跃线程数: " << pool.getActiveThreadCount() << std::endl;
    std::cout << "等待线程数: " << pool.getWaitingThreadCount() << std::endl;
    std::cout << "队列任务数: " << pool.getTaskCount() << std::endl;
    std::cout << "已完成任务数: " << pool.getCompletedTaskCount() << std::endl;
    std::cout << "失败任务数: " << pool.getFailedTaskCount() << std::endl;
}

void busyTask(int id) {
    std::cout << "[占位] 占位任务 " << id << " 开始执行（保持线程忙碌）" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 较长的执行时间
    std::cout << "[占位] 占位任务 " << id << " 执行完成" << std::endl;
}

int main() {
    printSeparator("C++11线程池实现 - 第六天测试");

     // 先验证优先级比较逻辑
        // std::cout << ">>> 验证优先级比较逻辑..." << std::endl;
        // TaskInfo lowTask(nullptr, TaskPriority::LOW, "low", "低优先级");
        // TaskInfo mediumTask(nullptr, TaskPriority::MEDIUM, "medium", "中优先级");
        // TaskInfo highTask(nullptr, TaskPriority::HIGH, "high", "高优先级");
        // TaskInfo criticalTask(nullptr, TaskPriority::CRITICAL, "critical", "关键优先级");
        
        // std::priority_queue<TaskInfo> testQueue;
        // testQueue.push(lowTask);
        // testQueue.push(mediumTask);
        // testQueue.push(highTask);
        // testQueue.push(criticalTask);
        // testQueue.push(lowTask);
        // testQueue.push(mediumTask);
        // testQueue.push(highTask);
        // testQueue.push(criticalTask);
        
        // std::cout << "优先级队列测试顺序：";
        // while (!testQueue.empty()) {
        //     std::cout << getPriorityString(testQueue.top().priority) << " ";
        //     testQueue.pop();
        // }
        // std::cout << std::endl;

    
    try {
        
        ThreadPool pool(4, LogLevel::INFO);  // 2，1
        // std::cout << "线程池创建完成，个工作线程" << std::endl;

        // 提交占位任务，确保让工作线程忙起来，更加直观的看到优先级顺序是否符合预期。
        for (int i = 0; i < 4; ++i) {
            pool.enqueue(busyTask, i);
        }

    // 等待确保所有线程都开始执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        printSeparator("优先级测试 - 连续提交所有任务");
        
        std::cout << "\n提交策略说明:" << std::endl;
        std::cout << "1. 在一个循环中连续提交所有优先级的任务" << std::endl;
        std::cout << "2. 提交顺序: 低→中→高→关键" << std::endl;
        std::cout << "3. 预期执行顺序: 关键→高→中→低" << std::endl;
        std::cout << "4. 使用2个工作线程，便于观察队列排序效果\n" << std::endl;

        // 存储所有任务的future
        std::vector<std::future<int>> allResults;
        
        // 在一个循环中连续提交所有任务，模拟同时到达的情况
        std::cout << ">>> 连续提交所有优先级任务..." << std::endl;
        
        // 提交顺序：低→中→高→关键，但执行应该是：关键→高→中→低
        struct TaskInfo {
            int id;
            TaskPriority priority;
            std::string desc;
        };
        
        std::vector<TaskInfo> tasks = {
            {1, TaskPriority::LOW, "低优先级任务1"},
            {2, TaskPriority::LOW, "低优先级任务2"},
            {3, TaskPriority::MEDIUM, "中优先级任务3"},
            {4, TaskPriority::MEDIUM, "中优先级任务4"},
            {5, TaskPriority::HIGH, "高优先级任务5"},
            {6, TaskPriority::HIGH, "高优先级任务6"},
            {7, TaskPriority::CRITICAL, "关键优先级任务7"},
            {8, TaskPriority::CRITICAL, "关键优先级任务8"},
            {9, TaskPriority::LOW, "低优先级任务9"},
            {10, TaskPriority::HIGH, "高优先级任务10"}
        };
        
        // 连续提交所有任务
        for (const auto& task : tasks) {
            allResults.push_back(
                pool.enqueueWithInfo(
                    "task-" + std::to_string(task.id),
                    task.desc,
                    task.priority,
                    simpleComputeTask, task.id, task.priority
                )
            );
        }

        printPoolStatus(pool, "所有任务提交后状态");
        
        std::cout << "\n>>> 观察任务执行顺序..." << std::endl;
        
        // 等待所有任务完成
        std::cout << "\n>>> 等待所有任务完成..." << std::endl;
        for (size_t i = 0; i < allResults.size(); ++i) {
            try {
                int result = allResults[i].get();
                // 这里不输出结果，专注观察执行顺序
            } catch (const std::exception& e) {
                std::cout << "任务 " << i << " 异常: " << e.what() << std::endl;
            }
        }
        #if 1
        printSeparator("测试任务ID和描述功能");

        auto taskWithId = pool.enqueueWithInfo(
            "special-task", 
            "这是一个带ID和描述的特殊任务", 
            TaskPriority::HIGH, 
            ioTask, "特殊任务", 200, TaskPriority::HIGH
        );

        // 等待特殊任务完成
        try {
            std::string result = taskWithId.get();
            std::cout << "特殊任务结果: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "特殊任务失败: " << e.what() << std::endl;
        }

        printSeparator("测试异常处理");
        
        std::vector<std::future<bool>> riskyResults;
        for (int i = 0; i < 4; ++i) {
            // 偶数ID的任务会失败
            bool shouldFail = (i % 2 == 0);
            std::cout << ">>> 提交风险任务 " << i << " (预期" << (shouldFail ? "失败" : "成功") << ")" << std::endl;
            riskyResults.push_back(
                pool.enqueueWithInfo(
                    "risky-" + std::to_string(i),
                    "可能失败的任务 " + std::to_string(i),
                    TaskPriority::MEDIUM,
                    riskyTask, i, shouldFail, TaskPriority::MEDIUM
                )
            );
        }
        
        // 获取风险任务结果
        std::cout << "\n>>> 风险任务结果:" << std::endl;
        for (size_t i = 0; i < riskyResults.size(); ++i) {
            try {
                bool result = riskyResults[i].get();// 如果业务函数抛出了异常，get方法会重新抛出异常，由下面的catch捕获。
                std::cout << "  ✓ 风险任务 " << i << " 结果: " << (result ? "成功" : "失败") << std::endl;
            } catch (const std::exception& e) {
                std::cout << "  ✗ 风险任务 " << i << " 异常: " << e.what() << std::endl;
            }
        }
        #endif
        printSeparator("测试线程池控制功能");
        
        // 等待所有任务完成
        pool.waitForTasks();
        printPoolStatus(pool, "所有任务完成后状态");
        
        // 测试暂停/恢复功能
        std::cout << "\n>>> 测试暂停/恢复功能..." << std::endl;
        
        // 提交一些任务
        std::cout << "提交暂停测试任务..." << std::endl;
        for (int i = 0; i < 3; ++i) {
            pool.enqueue(ioTask, "暂停测试-" + std::to_string(i), 200, TaskPriority::MEDIUM);
        }
        
        // 暂停线程池
        pool.pause();
        std::cout << "线程池已暂停" << std::endl;
        
        // 再提交一些任务（这些任务会被暂停）
        std::cout << "线程池暂停期间提交更多任务..." << std::endl;
        for (int i = 3; i < 6; ++i) {
            pool.enqueue(ioTask, "暂停测试-" + std::to_string(i), 200, TaskPriority::MEDIUM);
        }
        
        printPoolStatus(pool, "暂停后状态");
        
        std::cout << "等待2秒观察暂停效果..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // 恢复线程池
        pool.resume();
        std::cout << "线程池已恢复" << std::endl;
        
        // 等待所有任务完成
        pool.waitForTasks();
        
        printSeparator("最终性能报告");
        std::cout << pool.getMetricsReport() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "主程序异常: " << e.what() << std::endl;
        return 1;
    }
    
    printSeparator("第六天测试完成");
    return 0;
}