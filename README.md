# ThreadPool — 高性能 C++17 线程池库

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.10+-green.svg)](https://cmake.org)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

一个功能全面、生产就绪的 C++17 线程池实现，提供任务优先级调度、超时控制、动态线程调整、性能指标采集和任务生命周期管理等高级特性。

---

## 📋 目录

- [特性](#-特性)
- [快速开始](#-快速开始)
  - [构建要求](#构建要求)
  - [编译安装](#编译安装)
  - [在项目中使用](#在项目中使用)
- [API 参考](#-api-参考)
  - [ThreadPool](#threadpool-类)
  - [TaskInfo](#taskinfo-结构体)
  - [TaskPriority / TaskStatus / TaskFetchResult](#枚举)
  - [ThreadPoolMetrics](#threadpoolmetrics-结构体)
  - [Logger](#logger-类)
- [使用示例](#-使用示例)
  - [基础用法](#1-基础用法创建与提交)
  - [任务优先级](#2-任务优先级)
  - [任务取消](#3-任务取消)
  - [任务超时](#4-任务超时)
  - [批量提交](#5-批量提交任务)
  - [暂停/恢复](#6-暂停与恢复)
  - [动态调整线程数](#7-动态调整线程数)
  - [等待所有任务](#8-等待所有任务完成)
  - [性能监控](#9-获取性能报告)
- [构建与测试](#-构建与测试)
- [项目结构](#-项目结构)
- [许可证](#-许可证)

---

## ✨ 特性

| 特性 | 说明 |
|------|------|
| **🧵 固定 / 动态线程池** | 初始化时指定线程数，运行时可通过 `resize()` 动态调整 |
| **📊 四级任务优先级** | `LOW` / `MEDIUM` / `HIGH` / `CRITICAL`，同优先级 FIFO |
| **⏱️ 任务超时控制** | 每个任务可单独设定超时时间，超时自动标记失败 |
| **🆔 唯一任务 ID** | 可选的任务标识，支持去重检测和按 ID 查询 / 取消 |
| **❌ 任务取消** | 对 Waiting 状态的任务可按 ID 取消 |
| **⏸️ 暂停与恢复** | `pause()` / `resume()` 控制整个线程池的工作状态 |
| **🗑️ 清空队列** | `clearTasks()` 立即移除所有等待中的任务 |
| **📈 性能指标** | 内置 `ThreadPoolMetrics`，跟踪完成数、失败数、吞吐量、平均执行时间等 |
| **📝 可配置日志** | `Logger` 支持控制台 + 文件输出，日志级别可调 |
| **🛡️ 异常安全** | 任务内抛出的异常会通过 `std::future` 传递给调用者 |
| **🧩 批量提交** | `enqueueMany()` / `enqueueManyWithIdPrefix()` 批量提交同类任务 |
| **🔒 线程安全** | 所有公共方法均为线程安全 |

---

## 🚀 快速开始

### 构建要求

- **C++17** 兼容编译器（GCC 8+ / Clang 10+ / MSVC 2019+）
- **CMake** 3.10+
- **C++ Threads**（标准库支持）

### 编译安装

```bash
# 克隆项目
git clone https://github.com/your-username/threadpool.git
cd threadpool

# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
cmake --build .

# 运行测试（可选）
ctest

# 安装（可选，默认安装到 /usr/local）
cmake --install .
```

### 在项目中使用

**方法一：作为 CMake 子目录（推荐）**

```cmake
# CMakeLists.txt
add_subdirectory(path/to/threadpool)
target_link_libraries(your_target PRIVATE threadpool)
target_include_directories(your_target PRIVATE path/to/threadpool/include)
```

**方法二：安装后 find_package**

```cmake
# CMakeLists.txt
find_package(threadpool REQUIRED)
target_link_libraries(your_target PRIVATE threadpool::threadpool)
```

**方法三：直接包含源码**

将 `include/` 目录加入头文件搜索路径，并将 `src/` 下的源文件加入编译。

---

## 📚 API 参考

### ThreadPool 类

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `ThreadPool(size_t threads, LogLevel, bool consoleLog, string logFile)` | — | 构造函数，创建 `threads` 个工作线程 |
| `~ThreadPool()` | — | 析构函数，等待所有线程结束后清理 |
| `enqueue(F&& f, Args&&...)` | `std::future<return_type>` | 以 MEDIUM 优先级提交任务 |
| `enqueueWithPriority(priority, timeout, F&& f, Args&&...)` | `std::future<return_type>` | 带优先级和超时提交任务 |
| `enqueueWithInfo(taskId, desc, priority, timeout, F&& f, Args&&...)` | `std::future<return_type>` | 完整版 — 含 ID、描述、优先级、超时 |
| `enqueueMany(tasks, priority, timeout)` | `vector<future<void>>` | 批量提交 void 任务 |
| `enqueueManyWithIdPrefix(idPrefix, descPrefix, tasks, priority, timeout)` | `vector<future<void>>` | 带 ID 前缀的批量提交 |
| `setMaxThreads(size_t max)` | `void` | 设置最大线程数上限 |
| `getMaxThreads()` | `size_t` | 获取最大线程数 |
| `getThreadCount()` | `size_t` | 获取当前工作线程数 |
| `getActiveThreadCount()` | `size_t` | 获取当前活跃线程数 |
| `getWaitingThreadCount()` | `size_t` | 获取空闲线程数 |
| `getTaskCount()` | `size_t` | 获取队列中等待的任务数 |
| `getCompletedTaskCount()` | `size_t` | 获取已完成任务数 |
| `getFailedTaskCount()` | `size_t` | 获取失败任务数 |
| `cancelTask(const string& taskId)` | `bool` | 取消指定 ID 的任务 |
| `resize(size_t threads)` | `void` | 动态增减工作线程 |
| `pause()` | `void` | 暂停线程池（暂停分发新任务） |
| `resume()` | `void` | 恢复线程池 |
| `waitForTasks()` | `void` | 阻塞直到队列为空且无活跃线程 |
| `clearTasks()` | `void` | 清空任务队列（标记等待中任务为已取消） |
| `getMetricsReport()` | `string` | 获取性能指标报告 |
| `setLogLevel(LogLevel)` | `void` | 设置日志级别 |
| `isStopped()` | `bool` | 查询线程池是否已停止 |
| `getTaskStatus(const string& taskId)` | `TaskStatus` | 查询任务状态 |
| `getTaskStatusString(const string& taskId)` | `string` | 查询任务状态字符串 |

### TaskInfo 结构体

| 字段 | 类型 | 说明 |
|------|------|------|
| `task` | `function<void()>` | 任务可调用体 |
| `priority` | `TaskPriority` | 任务优先级 |
| `submitTime` | `time_point` | 提交时间戳 |
| `taskId` | `string` | 任务唯一标识（可选） |
| `description` | `string` | 任务描述 |
| `status` | `TaskStatus` | 当前状态 |
| `errorMessage` | `string` | 异常时的错误信息 |
| `timeout` | `milliseconds` | 超时时间（0 表示无限制） |

### 枚举

```cpp
enum class TaskPriority { LOW, MEDIUM, HIGH, CRITICAL };
enum class TaskStatus  { WAITING, RUNNING, COMPLETED, FAILED, CANCELED, NOT_FOUND };
enum class TaskFetchResult { SHOULD_EXIT, NO_TASK, HAS_TASK };
enum class LogLevel    { NONE = 0, ERROR = 1, WARN = 2, INFO = 3, DEBUG = 4 };
```

### ThreadPoolMetrics 结构体

| 方法 / 字段 | 说明 |
|-------------|------|
| `totalTasks` | 总提交任务数 |
| `completedTasks` | 成功完成任务数 |
| `failedTasks` | 失败任务数 |
| `canceledTasks` | 取消任务数 |
| `timedOutTasks` | 超时任务数 |
| `activeThreads` | 当前活跃线程数 |
| `peakThreads` | 峰值活跃线程数 |
| `peakQueueSize` | 峰值队列大小 |
| `getAverageTaskTime()` | 平均任务执行时间（毫秒） |
| `getUptime()` | 线程池运行时间（秒） |
| `getThroughput()` | 吞吐量（任务/秒） |
| `getReport()` | 获取完整报告字符串 |

### Logger 类

| 方法 | 说明 |
|------|------|
| `log(LogLevel, message)` | 写日志 |
| `setLevel(LogLevel)` | 设置日志级别 |
| `setLogFile(filename)` | 设置日志文件路径 |
| `setConsoleOutput(bool)` | 启用 / 禁用控制台输出 |

---

## 💡 使用示例

### 1. 基础用法：创建与提交

```cpp
#include "ThreadPool.h"
#include <iostream>

int main() {
    // 创建 4 个工作线程的线程池
    ThreadPool pool(4);

    // 提交无参数任务
    auto future = pool.enqueue([] {
        return 42;
    });

    // 提交带参数任务
    auto result = pool.enqueue([](int a, int b) {
        return a + b;
    }, 3, 4);

    std::cout << "结果: " << future.get() << std::endl;  // 42
    std::cout << "求和: " << result.get() << std::endl;   // 7

    return 0;
}
```

### 2. 任务优先级

```cpp
#include "ThreadPool.h"

ThreadPool pool(4);

// 不同优先级的任务 —— CRITICAL 优先执行
auto low    = pool.enqueueWithPriority(TaskPriority::LOW,     0ms, []{ /* ... */ });
auto high   = pool.enqueueWithPriority(TaskPriority::HIGH,    0ms, []{ /* ... */ });
auto urgent = pool.enqueueWithPriority(TaskPriority::CRITICAL, 0ms, []{ /* ... */ });

// 同优先级按 FIFO 顺序执行
```

### 3. 任务取消

```cpp
#include "ThreadPool.h"
#include <thread>

ThreadPool pool(4);

// 提交可取消任务（需使用唯一 ID）
pool.enqueueWithInfo("cancelable-task", "随时可取消",
                     TaskPriority::LOW, 0ms, []{
    std::this_thread::sleep_for(10s);
});

std::this_thread::sleep_for(100ms);

// 按 ID 取消
bool ok = pool.cancelTask("cancelable-task");
if (ok) {
    std::cout << "任务已取消" << std::endl;
}

// 查询状态
std::cout << pool.getTaskStatusString("cancelable-task") << std::endl;
// 输出："已取消"
```

### 4. 任务超时

```cpp
#include "ThreadPool.h"
#include <thread>

ThreadPool pool(4);

// 提交一个会在 500ms 后超时的任务
auto future = pool.enqueueWithInfo("timeout-task", "会超时的任务",
                                   TaskPriority::HIGH, 500ms, []{
    std::this_thread::sleep_for(2s);  // 运行 2 秒
});

try {
    future.get();  // 等待结果
} catch (const std::exception& e) {
    // 捕获超时异常
    std::cerr << "任务超时: " << e.what() << std::endl;
}
```

### 5. 批量提交任务

```cpp
#include "ThreadPool.h"
#include <vector>

ThreadPool pool(4);

// 准备一批任务
std::vector<std::function<void()>> tasks;
for (int i = 0; i < 10; ++i) {
    tasks.push_back([i] {
        // 执行任务...
    });
}

// 方式一：普通批量提交
auto futures = pool.enqueueMany(tasks);

// 方式二：带 ID 前缀的批量提交
auto futures2 = pool.enqueueManyWithIdPrefix("batch-job", "批量任务",
                                              tasks, TaskPriority::HIGH, 0ms);

// 等待所有批量任务完成
for (auto& f : futures) {
    f.get();
}
```

### 6. 暂停与恢复

```cpp
#include "ThreadPool.h"
#include <thread>

ThreadPool pool(4);

// 提交一批任务
for (int i = 0; i < 10; ++i) {
    pool.enqueue([i]{ std::this_thread::sleep_for(200ms); });
}

// 暂停 —— 已执行的任务继续，队列中的任务暂停分发
pool.pause();

// 提交更多的任务（入列但不会执行）
pool.enqueue([]{ /* ... */ });

// 恢复 —— 队列中的任务继续分发执行
pool.resume();
```

### 7. 动态调整线程数

```cpp
#include "ThreadPool.h"

ThreadPool pool(4);  // 初始 4 个线程

// 增加线程
pool.resize(8);  // 扩展到 8 个线程

// 减少线程
pool.resize(4);  // 缩减回 4 个线程

// 设置最大线程上限
pool.setMaxThreads(32);

// 设置小于当前线程数会抛出异常
try {
    pool.setMaxThreads(2);  // 当前 4 线程 > 2
} catch (const std::exception& e) {
    // "Cannot set max threads less than current thread count"
}
```

### 8. 等待所有任务完成

```cpp
#include "ThreadPool.h"

ThreadPool pool(4);

for (int i = 0; i < 100; ++i) {
    pool.enqueue([i]{ /* 耗时任务 */ });
}

// 阻塞直到所有任务完成
pool.waitForTasks();

std::cout << "所有 100 个任务已完成!" << std::endl;
```

### 9. 获取性能报告

```cpp
#include "ThreadPool.h"
#include <iostream>

ThreadPool pool(4);

// ... 提交并执行大量任务 ...

// 获取完整性能报告
std::cout << pool.getMetricsReport() << std::endl;
// 输出示例：
//   线程池性能报告:
//     运行时间: 12.345 秒
//     总任务数: 1000
//     已完成任务数: 998
//     失败任务数: 2
//     当前活跃线程数: 0
//     峰值活跃线程数: 4
//     峰值队列大小: 50
//     平均任务执行时间: 15.2 毫秒
//     任务吞吐量: 80.9 任务/秒
```

---

## 🧪 构建与测试

项目内置了 7 个测试用例，覆盖核心功能：

```bash
# 完整构建
mkdir build && cd build
cmake ..
cmake --build .

# 运行所有测试
ctest

# 或单独运行
./test/test_day1_basic   # 基础创建/销毁
./test/test_day2_basic   # 任务提交/执行
./test/test_day3_basic   # （功能测试）
./test/test_day4_basic   # （功能测试）
./test/test_day5_basic   # 暂停/恢复/清空/异常/动态调整
./test/test_day7_basic   # 唯一ID/取消/超时/批量/优先级/性能报告
```

测试覆盖：
| 测试套件 | 覆盖特性 |
|----------|----------|
| `test1` | 线程池创建与销毁 |
| `test5` | 暂停/恢复、动态 resize、异常传播、清空队列、等待完成 |
| `test7` | 唯一 ID、重复 ID 拒绝、任务取消、超时控制、批量提交、带前缀批量、混合优先级、性能报告 |

---

## 📁 项目结构

```
threadpool/
├── CMakeLists.txt              # 根 CMake 配置
├── include/                    # 公共头文件
│   ├── Logger.h                # 日志系统
│   ├── TaskInfo.h              # 任务元数据结构
│   ├── ThreadPool.h            # 线程池主类（声明）
│   ├── ThreadPool.inl          # 线程池模板方法实现
│   └── ThreadPoolMetrics.h     # 性能指标结构体
├── src/                        # 实现文件
│   ├── CMakeLists.txt          # 库构建配置
│   ├── Logger.cpp              # 日志实现
│   ├── TaskInfo.cpp            # 任务信息实现
│   ├── ThreadPool.cpp          # 线程池核心实现
│   └── ThreadPoolMetrics.cpp   # 性能指标实现
└── test/                       # 测试用例
    ├── CMakeLists.txt
    ├── test1.cpp               # 基础创建/销毁
    ├── test2.cpp               # 任务提交
    ├── test3.cpp               # 功能测试
    ├── test4.cpp               # 功能测试
    ├── test5.cpp               # 控制功能综合测试
    ├── test6.cpp               # （可选）
    └── test7.cpp               # 高级特性综合测试
```

---

## ⚙️ 深入理解架构

### 任务调度流程

```
enqueue*(F, args...)
  │
  ├─→ 创建 std::promise + std::future
  ├─→ 包装为 std::function<void()>
  ├─→ 创建 TaskInfo（含优先级、ID、超时等）
  ├─→ 推入 std::priority_queue<TaskInfo>
  └─→ 通知一个空闲工作线程
         │
         ▼
workerThread(id)
  ├─→ getNextTask() — 获取最高优先级任务
  │     ├─ 跳过已取消的任务
  │     └─ 无任务时 condition_variable 等待
  ├─→ executeTask() — 执行
  │     ├─ 有超时 → std::async + wait_for
  │     └─ 无超时 → 直接执行
  ├─→ 异常 → handleTaskException()
  └─→ 清理 → cleanupTask()
```

### 线程安全设计

- 所有对任务队列的访问互斥于 `queue_mutex`
- 线程等待 / 唤醒通过 `condition_variable`
- `stop` / `paused` 使用 `std::atomic<bool>` 实现无锁状态查询
- `ThreadPoolMetrics` 全部使用 `std::atomic` 计数器，无锁更新
- 析构时先设置 `stop = true` 再 `notify_all`，确保线程有序退出

### 超时实现原理

当任务的 `timeout > 0` 时，`executeTaskWithTimeout()` 使用 `std::async(std::launch::async, ...)` 将任务提交给另一个线程，然后以 `future.wait_for(timeout)` 等待：

- **超时**：`wait_for` 返回 `future_status::timeout`，抛出运行时异常
- **正常完成**：`future.get()` 传播任务中抛出的异常

> **注意**：超时后的底层线程仍会运行到结束点，但主线程会立即收到超时通知并继续处理后续任务。

---

## 📝 日志配置

```cpp
// 创建线程池时配置日志
ThreadPool pool(4,
    LogLevel::INFO,              // 日志级别
    true,                        // 启用控制台输出
    "threadpool.log"             // 日志文件路径（空字符串 = 不写文件）
);

// 运行时动态调整
pool.setLogLevel(LogLevel::DEBUG);  // 切换到调试级别
```

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交修改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 打开 Pull Request

### 开发指引

- 遵循现有的代码风格（RAII、智能指针、`std::atomic` 无锁设计）
- 新特性请附带对应的测试用例
- 确保 `ctest` 全部通过
- 保持 C++17 兼容，不引入外部依赖

---

## 📄 许可证

本项目基于 MIT 许可证开源。详见 [LICENSE](LICENSE) 文件。

---

<p align="center">
  <b>ThreadPool</b> — 简洁 · 高效 · <a href="test/test7.cpp">生产就绪</a>
</p>
