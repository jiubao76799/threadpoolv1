#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>
#include <thread>
#include <cmath>
#include <atomic>
#include "ThreadPool.h"

int computeTask(int id, int complexity) {
    auto start = std::chrono::steady_clock::now();
    
    // 修复计算逻辑
    long long result = 0;
    for (int i = 1; i <= complexity * 100000; ++i) {
        result += i * i;  // 简单但有意义的计算
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "计算任务 " << id << " 完成，耗时: " << duration.count() << "us" << std::endl;
   std::cout<<result << std::endl;
    return static_cast<int>(result % 10000);  // 返回有意义的结果
}
// 模拟IO任务
std::string ioTask(const std::string& name, int delay) {
    std::cout << "IO任务 " << name << " 开始，模拟延迟: " << delay << "ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    std::cout << "IO任务 " << name << " 完成" << std::endl;
    return "IO结果: " + name;
}

// 模拟长时间运行的任务（用于测试超时）
void longRunningTask(int id, int duration) {
    std::cout << "长时间任务 " << id << " 开始，预计运行 " << duration << "ms" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
    std::cout << "长时间任务 " << id << " 完成" << std::endl;
}

// 模拟可被取消的任务
void cancellableTask(int id, std::atomic<bool>& shouldStop) {
    std::cout << "可取消任务 " << id << " 开始" << std::endl;
    
    for (int i = 0; i < 100; ++i) {
        if (shouldStop.load()) {
            std::cout << "可取消任务 " << id << " 被取消" << std::endl;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "可取消任务 " << id << " 完成" << std::endl;
}

// 简单的批量任务
void batchTask(int id) {
    std::cout << "批量任务 " << id << " 执行中..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + id * 10));
    std::cout << "批量任务 " << id << " 完成" << std::endl;
}

// 打印分隔线
void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "=== " << title << " ===" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

  void printPoolStatus(ThreadPool &pool){
            std::cout << "当前状态:" << std::endl;
            std::cout << "  线程数: " << pool.getThreadCount() << std::endl;
            std::cout << "  最大线程数: " << pool.getMaxThreads() << std::endl;
            std::cout << "  活跃线程数: " << pool.getActiveThreadCount() << std::endl;
            std::cout << "  队列任务数: " << pool.getTaskCount() << std::endl;
            std::cout << "  已完成任务数: " << pool.getCompletedTaskCount() << std::endl;
            std::cout << "  失败任务数: " << pool.getFailedTaskCount() << std::endl;
  }
           
        

int main() {
    std::cout << "=== C++11线程池实现 - 第七天高级功能测试 ===" << std::endl;
    
    try {
        // 创建线程池
        ThreadPool pool(4, LogLevel::INFO, true, "threadpool.log");
        std::cout << "线程池创建完成，4个工作线程，最大线程数: " << pool.getMaxThreads() << std::endl;

        // ==================== 测试1: 任务唯一性保证 ====================
        printSeparator("测试1: 任务唯一性保证");
        
        // 提交具有唯一ID的任务
        auto future1 = pool.enqueueWithInfo("unique-task-1", "第一个唯一任务", 
                                           TaskPriority::MEDIUM, std::chrono::milliseconds(0),
                                           ioTask, "Task1", 200);
        
        auto future2 = pool.enqueueWithInfo("unique-task-2", "第二个唯一任务", 
                                           TaskPriority::MEDIUM, std::chrono::milliseconds(0),
                                           ioTask, "Task2", 200);
        
        // 尝试提交重复ID的任务（应该抛出异常）
        try {
            auto future3 = pool.enqueueWithInfo("unique-task-1", "重复ID任务", 
                                               TaskPriority::HIGH, std::chrono::milliseconds(0),
                                               ioTask, "Duplicate", 100);
            std::cout << "错误：重复ID任务应该被拒绝" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "正确：重复ID任务被拒绝 - " << e.what() << std::endl;
        }
        
        // 等待任务完成
        std::string result1 = future1.get();
        std::string result2 = future2.get();
        std::cout << "唯一任务结果: " << result1 << std::endl;
        std::cout << "唯一任务结果: " << result2 << std::endl;

        // ==================== 测试2: 任务取消机制 ====================
        printSeparator("测试2: 任务取消机制");
        
        // 提交几个任务
        pool.enqueueWithInfo("task-1", "任务1", 
                                           TaskPriority::MEDIUM, std::chrono::milliseconds(0),
                                           ioTask, "Task1", 200);
            
        pool.enqueueWithInfo("task-2", "任务2", 
                                           TaskPriority::MEDIUM, std::chrono::milliseconds(0),
                                           ioTask, "Task1", 200);
        pool.enqueueWithInfo("task-3", "任务3", 
                                           TaskPriority::MEDIUM, std::chrono::milliseconds(0),
                                           ioTask, "Task1", 200);

        pool.enqueueWithInfo("cancel-task-1", "将被取消的任务1", 
                           TaskPriority::LOW, std::chrono::milliseconds(0),
                           ioTask, "Cancel1", 1000);
        
        pool.enqueueWithInfo("cancel-task-2", "将被取消的任务2", 
                           TaskPriority::LOW, std::chrono::milliseconds(0),
                           ioTask, "Cancel2", 1000);
        
        pool.enqueueWithInfo("normal-task", "正常任务", 
                           TaskPriority::HIGH, std::chrono::milliseconds(0),
                           ioTask, "Normal", 200);
        
        // 等待一小段时间，让正常任务开始执行
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 尝试取消任务
        bool canceled1 = pool.cancelTask("cancel-task-1");
        bool canceled2 = pool.cancelTask("cancel-task-2");
        bool canceled3 = pool.cancelTask("non-existent-task");
        
        std::cout << "取消 cancel-task-1: " << (canceled1 ? "成功" : "失败") << std::endl;
        std::cout << "取消 cancel-task-2: " << (canceled2 ? "成功" : "失败") << std::endl;
        std::cout << "取消 non-existent-task: " << (canceled3 ? "不应成功" : "正确拒绝") << std::endl;
        
        // 检查任务状态
        std::cout << "cancel-task-1 状态: " << pool.getTaskStatusString("cancel-task-1") << std::endl;
        std::cout << "cancel-task-2 状态: " << pool.getTaskStatusString("cancel-task-2") << std::endl;
        
        // 等待剩余任务完成
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // ==================== 测试3: 任务超时处理 ====================
        printSeparator("测试3: 任务超时处理");
        
        // 提交一个会超时的任务
        auto timeoutFuture = pool.enqueueWithInfo("timeout-task", "超时任务", 
                                                 TaskPriority::HIGH, std::chrono::milliseconds(500),
                                                 longRunningTask, 1, 1000);  // 任务需要1000ms，但超时设置为500ms
        
        // 提交一个不会超时的任务
        auto normalFuture = pool.enqueueWithInfo("normal-timeout-task", "正常超时任务", 
                                                TaskPriority::HIGH, std::chrono::milliseconds(1000),
                                                longRunningTask, 2, 300);   // 任务需要300ms，超时设置为1000ms
        
        // 提交一个无超时限制的任务
        auto noTimeoutFuture = pool.enqueueWithInfo("no-timeout-task", "无超时任务", 
                                                   TaskPriority::HIGH, std::chrono::milliseconds(0),
                                                   longRunningTask, 3, 400);   // 无超时限制
        
        // 检查超时任务
        try {
            timeoutFuture.get();
            std::cout << "错误：超时任务应该抛出异常" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "正确：超时任务抛出异常 - " << e.what() << std::endl;
        }
        
        // 检查正常任务
        try {
            normalFuture.get();
            std::cout << "正确：正常超时任务成功完成" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "错误：正常超时任务不应失败 - " << e.what() << std::endl;
        }
        
        // 检查无超时任务
        try {
            noTimeoutFuture.get();
            std::cout << "正确：无超时任务成功完成" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "错误：无超时任务不应失败 - " << e.what() << std::endl;
        }

        // ==================== 测试4: 批量任务提交 ====================
        printSeparator("测试4: 批量任务提交");
        
        // 准备批量任务
        std::vector<std::function<void()>> batchTasks;
        for (int i = 0; i < 5; ++i) {
            batchTasks.push_back([i]() { batchTask(i); });
        }
        
        // 批量提交任务
        std::cout << "批量提交5个任务..." << std::endl;
        auto batchFutures = pool.enqueueMany(batchTasks, TaskPriority::MEDIUM, std::chrono::milliseconds(0));
        
        // 等待批量任务完成
        for (size_t i = 0; i < batchFutures.size(); ++i) {
            try {
                batchFutures[i].get();
               // std::cout << "批量任务 " << i << " 完成" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "批量任务 " << i << " 失败: " << e.what() << std::endl;
            }
        }

        // ==================== 测试5: 带ID前缀的批量任务提交 ====================
        printSeparator("测试5: 带ID前缀的批量任务提交");
        
        // 准备另一组批量任务
        std::vector<std::function<void()>> prefixTasks;
        for (int i = 0; i < 3; ++i) {
            prefixTasks.push_back([i]() { 
                std::cout << "前缀任务 " << i << " 执行中..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                std::cout << "前缀任务 " << i << " 完成" << std::endl;
            });
        }
       
        // 使用ID前缀批量提交
        std::cout << "使用ID前缀批量提交3个任务..." << std::endl;
        auto prefixFutures = pool.enqueueManyWithIdPrefix("batch", "批量任务", prefixTasks, 
                                                         TaskPriority::HIGH, std::chrono::milliseconds(0));
        
        // 等待前缀任务完成
        for (size_t i = 0; i < prefixFutures.size(); ++i) {
            try {
                prefixFutures[i].get();
               // std::cout << "前缀任务 batch-" << i << " 完成" << std::endl;
            } catch (const std::exception& e) {
               // std::cout << "前缀任务 batch-" << i << " 失败: " << e.what() << std::endl;
            }
        }

        // ==================== 测试6: 混合功能测试 ====================
        printSeparator("测试6: 混合功能测试");
        
        // 提交不同优先级和超时设置的任务
        auto highPriorityFuture = pool.enqueueWithInfo("high-priority", "高优先级任务", 
                                                      TaskPriority::HIGH, std::chrono::milliseconds(0),
                                                      computeTask, 100, 2);
        
        auto mediumTimeoutFuture = pool.enqueueWithInfo("medium-timeout", "中优先级超时任务", 
                                                       TaskPriority::MEDIUM, std::chrono::milliseconds(800),
                                                       longRunningTask, 200, 500);
        
        auto lowPriorityFuture = pool.enqueueWithInfo("low-priority", "低优先级任务", 
                                                     TaskPriority::LOW, std::chrono::milliseconds(0),
                                                     ioTask, "LowPriority", 300);
        
        // 尝试取消一个任务
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        bool cancelResult = pool.cancelTask("low-priority");
        std::cout << "取消低优先级任务: " << (cancelResult ? "成功" : "失败") << std::endl;
        
        // 获取结果
        try {
            int result1 = highPriorityFuture.get();
            std::cout << "高优先级任务结果: " << result1 << std::endl;
        } catch (const std::exception& e) {
            std::cout << "高优先级任务异常: " << e.what() << std::endl;
        }
        
        try {
            mediumTimeoutFuture.get();
            std::cout << "中优先级超时任务完成" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "中优先级超时任务异常: " << e.what() << std::endl;
        }
        
        try {
            std::string result3 = lowPriorityFuture.get();
            std::cout << "低优先级任务结果: " << result3 << std::endl;
        } catch (const std::exception& e) {
            std::cout << "低优先级任务异常: " << e.what() << std::endl;
        }

        // ==================== 测试7: 线程池控制功能 ====================
        printSeparator("测试7: 线程池控制功能");
        
        // 显示当前状态
        printPoolStatus(pool);
        // 测试最大线程数设置
        try {
            pool.setMaxThreads(20);
            std::cout << "设置最大线程数为20成功" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "设置最大线程数失败: " << e.what() << std::endl;
        }
        
        // 尝试设置无效的最大线程数
        try {
            pool.setMaxThreads(2);  // 小于当前线程数
            std::cout << "错误：应该拒绝小于当前线程数的设置" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "正确：拒绝无效的最大线程数设置 - " << e.what() << std::endl;
        }
        
        // 测试动态调整线程数
        std::cout << "调整线程数到6..." << std::endl;
        pool.resize(6);
        std::cout << "新的线程数: " << pool.getThreadCount() << std::endl;
        
        // 提交一些任务测试新的线程数
        for (int i = 0; i < 8; ++i) {
            pool.enqueue(ioTask, "ResizeTest-" + std::to_string(i), 100);
        }
        
        printPoolStatus(pool);
        // 等待任务完成
        pool.waitForTasks();
        
        // 调整回原来的线程数
        std::cout << "调整线程数回到4..." << std::endl;
        pool.resize(8);
        std::cout << "当前线程数: " << pool.getThreadCount() << std::endl;
        printPoolStatus(pool);

        // ==================== 测试8: 性能指标和报告 ====================
        printSeparator("测试8: 性能指标和报告");
        
        // 提交一些任务来生成性能数据
        std::vector<std::future<int>> performanceFutures;
        for (int i = 0; i < 100; ++i) {
            performanceFutures.push_back(
                pool.enqueueWithInfo("perf-task-" + std::to_string(i), "性能测试任务", 
                                   TaskPriority::MEDIUM, std::chrono::milliseconds(0),
                                   computeTask, i, 1)
            );
        }
        
        // 等待性能测试任务完成
        for (auto& future : performanceFutures) {
            try {
                future.get();
            } catch (const std::exception& e) {
                std::cout << "性能测试任务异常: " << e.what() << std::endl;
            }
        }
        
        // 显示最终性能报告
        std::cout << "\n最终性能报告:" << std::endl;
        std::cout << pool.getMetricsReport() << std::endl;

        // 等待所有任务完成
        pool.waitForTasks();
        
    } catch (const std::exception& e) {
        std::cerr << "主程序异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}