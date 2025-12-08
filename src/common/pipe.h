#pragma once

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <sys/wait.h>

class PipeCommandExecutor {
private:
    // 线程池相关成员
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    
    // 私有构造函数
    PipeCommandExecutor(size_t num_threads = std::thread::hardware_concurrency())
        : stop(false) {
        if (num_threads == 0) {
            num_threads = 4;
        }
        
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                this->workerThread();
            });
        }
    }
    
    // 工作线程函数
    void workerThread() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->condition.wait(lock, [this] {
                    return this->stop || !this->tasks.empty();
                });
                if (this->stop && this->tasks.empty()) return;
                task = std::move(this->tasks.front());
                this->tasks.pop();
            }
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Error in worker thread: " << e.what() << std::endl;
            }
        }
    }
    
    // 安全执行命令并返回结果
    std::string executeCommandImpl(const std::string& command) {
        FILE* pipe = nullptr;
        std::string result;
        
        try {
            pipe = popen(command.c_str(), "r");
            if (!pipe) {
                throw std::runtime_error("Failed to open pipe for command: " + command);
            }
            
            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            
            // 关闭管道并获取退出状态
            int status = pclose(pipe);
            pipe = nullptr; // 避免双重释放
            
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0) {
                    result += "\n[Exit code: " + std::to_string(exit_code) + "]";
                }
            } else if (WIFSIGNALED(status)) {
                result += "\n[Process terminated by signal: " + 
                         std::to_string(WTERMSIG(status)) + "]";
            }
        } catch (...) {
            // 发生异常时确保管道被关闭
            if (pipe) {
                pclose(pipe);
            }
            throw;
        }
        
        return result;
    }
    
    // 执行命令并返回结果和退出状态
    std::pair<std::string, int> executeCommandWithStatusImpl(const std::string& command) {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Failed to open pipe for command: " + command);
        }
        
        std::string result;
        char buffer[4096];
        
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        // 获取命令退出状态
        int status = pclose(pipe);
        
        if (WIFEXITED(status)) {
            return {result, WEXITSTATUS(status)};
        } else {
            return {result, -1}; // 进程被信号终止
        }
    }
    
public:
    // 禁止拷贝和移动
    PipeCommandExecutor(const PipeCommandExecutor&) = delete;
    PipeCommandExecutor& operator=(const PipeCommandExecutor&) = delete;
    
    ~PipeCommandExecutor() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    // 获取单例实例 - 使用局部静态变量
    static PipeCommandExecutor& getInstance() {
        static PipeCommandExecutor instance;
        return instance;
    }
    
    // 异步执行命令
    static std::future<std::string> executeAsync(const std::string& command) {
        auto& executor = getInstance();
        
        auto task = std::make_shared<std::packaged_task<std::string()>>(
            [&executor, command]() -> std::string {
                return executor.executeCommandImpl(command);
            }
        );
        
        std::future<std::string> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(executor.queue_mutex);
            if (executor.stop) {
                throw std::runtime_error("PipeCommandExecutor is shutting down");
            }
            executor.tasks.emplace([task]() { (*task)(); });
        }
        
        executor.condition.notify_one();
        return result;
    }
    
    // 异步执行命令并获取退出状态
    static std::future<std::pair<std::string, int>> executeAsyncWithStatus(const std::string& command) {
        auto& executor = getInstance();
        
        auto task = std::make_shared<std::packaged_task<std::pair<std::string, int>()>>(
            [&executor, command]() -> std::pair<std::string, int> {
                return executor.executeCommandWithStatusImpl(command);
            }
        );
        
        std::future<std::pair<std::string, int>> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(executor.queue_mutex);
            if (executor.stop) {
                throw std::runtime_error("PipeCommandExecutor is shutting down");
            }
            executor.tasks.emplace([task]() { (*task)(); });
        }
        
        executor.condition.notify_one();
        return result;
    }
    
    // 同步执行命令
    static std::string execute(const std::string& command) {
        return executeAsync(setCommand(command)).get();
    }
    
    // 同步执行命令并获取退出状态
    static std::pair<std::string, int> executeWithStatus(const std::string& command) {
        return executeAsyncWithStatus(setCommand(command)).get();
    }
    
    // 批量执行命令
    static std::vector<std::future<std::string>> executeBatch(
        const std::vector<std::string>& commands) {
        
        std::vector<std::future<std::string>> futures;
        futures.reserve(commands.size());
        
        for (const auto& cmd : commands) {
            futures.push_back(executeAsync(setCommand(cmd)));
        }
        
        return futures;
    }
    
    static std::string setCommand(const std::string& command) {
        return command + " 2>&1";
    }

    static void setThreadPoolSize(size_t num_threads) {
        auto& executor = getInstance();
        
        // 停止现有线程池
        executor.stop = true;
        executor.condition.notify_all();
        
        // 等待所有工作线程结束
        for (std::thread& worker : executor.workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        // 清空任务队列
        {
            std::unique_lock<std::mutex> lock(executor.queue_mutex);
            std::queue<std::function<void()>> empty;
            std::swap(executor.tasks, empty);
        }
        
        // 重置标志并创建新线程
        executor.stop = false;
        executor.workers.clear();
        
        for (size_t i = 0; i < num_threads; ++i) {
            executor.workers.emplace_back([&executor] {
                executor.workerThread();
            });
        }
    }
    
    // 关闭线程池
    static void shutdown() {
        auto& executor = getInstance();
        executor.stop = true;
        executor.condition.notify_all();
        
        for (std::thread& worker : executor.workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        // 清空任务队列
        {
            std::unique_lock<std::mutex> lock(executor.queue_mutex);
            std::queue<std::function<void()>> empty;
            std::swap(executor.tasks, empty);
        }
    }
    
    // 等待所有任务完成
    static void waitForCompletion() {
        auto& executor = getInstance();
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::unique_lock<std::mutex> lock(executor.queue_mutex);
            if (executor.tasks.empty()) {
                break;
            }
        }
    }
};