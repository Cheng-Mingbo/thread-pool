#include <iostream>
#include <chrono>
#include <random>
#include "thread_pool.h"
// 压测任务函数
int task(int id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100, 500);
    
    int sleep_time = dis(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    return id;
}

void stressTest(ThreadPool& thread_pool, int num_tasks) {
    using namespace std::chrono;
    
    // 记录开始时间
    auto start_time = high_resolution_clock::now();
    
    // 提交任务
    std::vector<std::future<int>> results;
    for (int i = 0; i < num_tasks; ++i) {
        results.emplace_back(thread_pool.enqueue(task, i));
    }
    
    // 等待任务完成并获取结果
    for (int i = 0; i < num_tasks; ++i) {
        results[i].wait();
        std::cout << "Task " << results[i].get() << " completed" << std::endl;
    }
    
    // 计算并输出总耗时
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time).count();
    std::cout << "Total time: " << duration << " ms" << std::endl;
}

void singleThreadTest(int num_tasks) {
    using namespace std::chrono;
    
    // 记录开始时间
    auto start_time = high_resolution_clock::now();
    
    // 顺序执行任务
    for (int i = 0; i < num_tasks; ++i) {
        int result = task(i);
        std::cout << "Task " << result << " completed" << std::endl;
    }
    
    // 计算并输出总耗时
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time).count();
    std::cout << "Total time: " << duration << " ms" << std::endl;
}

int main() {
    const int num_tasks = 100;
    
    std::cout << "Single-threaded test:" << std::endl;
    singleThreadTest(num_tasks);
    
    std::cout << "\nThread pool test:" << std::endl;
    ThreadPool thread_pool(4, 8);
    stressTest(thread_pool, num_tasks);
    
    return 0;
}

