//
// Created by Cheng MingBo on 2023/4/29.
//

#ifndef THREAD_POOL_THREAD_POOL_H
#define THREAD_POOL_THREAD_POOL_H
#include <iostream>
#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <chrono>

class ThreadPool {
  public:
    ThreadPool(size_t min_threads, size_t max_threads);
    ~ThreadPool();
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;
  
  private:
    void workerFunction();
    void adjustThreads();
    
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic_bool stop;
    
    size_t min_threads;
    size_t max_threads;
    
    std::thread adjuster_thread;
};

ThreadPool::ThreadPool(size_t min_threads, size_t max_threads)
        : stop(false), min_threads(min_threads), max_threads(max_threads)
{
    for (size_t i = 0; i < min_threads; ++i) {
        workers.emplace_back(&ThreadPool::workerFunction, this);
    }
    adjuster_thread = std::thread(&ThreadPool::adjustThreads, this);
}

ThreadPool::~ThreadPool()
{
    stop = true;
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
    adjuster_thread.join();
}


template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        
        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

void ThreadPool::workerFunction() {
    while (!stop) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->condition.wait(lock,
                                 [this] { return this->stop || !this->tasks.empty(); });
            if (this->stop && this->tasks.empty())
                return;
            task = std::move(this->tasks.front());
            this->tasks.pop();
        }
        
        task();
    }
}

void ThreadPool::adjustThreads() {
    while (!stop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            if (stop) break;
            
            size_t task_count = tasks.size();
            size_t worker_count = workers.size();
            
            // std::cout << "Current task count: " << task_count << std::endl;
            // std::cout << "Current worker count: " << worker_count << std::endl;
            
            if (task_count > worker_count && worker_count < max_threads) {
                // 增加线程
                workers.emplace_back(&ThreadPool::workerFunction, this);
                // std::cout << "Increased worker count to: " << workers.size() << std::endl;
            } else if (task_count < worker_count && worker_count > min_threads) {
                // 减少线程
                stop = true;
                condition.notify_one();
                workers.back().join();
                workers.pop_back();
                stop = false;
                // std::cout << "Decreased worker count to: " << workers.size() << std::endl;
            }
        }
    }
}



#endif //THREAD_POOL_THREAD_POOL_H
