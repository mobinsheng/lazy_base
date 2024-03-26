#ifndef _TASK_QUEUE_H_2024_
#define _TASK_QUEUE_H_2024_
/*
 *  Created by mobinsheng.
 */
#pragma once
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <future>
#include <chrono>

namespace lazy {

// task class
class Task {
public:
    Task() {}
    virtual ~Task() {}
    virtual void run() = 0;
    
    // call run
    void operator()() { return run(); }
    
    // is sync task
    bool is_sync = false;
    
    // is task execute finish
    bool finished = false;
    
    int64_t invoke_ms = 0;
    
private:
    Task(const Task&) = delete;
};

// An task wrapped a functor/closure
template <class ReturnT, class Closure>
class ClosureTask : public Task {
public:
    explicit ClosureTask(Closure&& closure) : closure_(std::forward<Closure>(closure)) {}
    
    virtual void run() override { result_ = closure_(); }
    
    const ReturnT& result() const {return result_;}
    
    ReturnT& result() {return result_;}
    
    ReturnT move_result() {return std::move(result_);}
    
private:
    Closure closure_;
    ReturnT result_;
};

// An task wrapped a functor/closure for void type
template <class Closure>
class ClosureTask<void, Closure> : public Task {
public:
    explicit ClosureTask(Closure&& closure) : closure_(std::forward<Closure>(closure)) {}
    
    virtual void run() override { closure_(); }
    
    void result() const {}
    
    void move_result() {}
    
private:
    Closure closure_;
};


// create Closure
template <class ReturnT, class Closure>
static std::unique_ptr<ClosureTask<ReturnT, Closure>> NewTask(Closure&& closure) {
    std::unique_ptr<ClosureTask<ReturnT, Closure>> ptr(new ClosureTask<ReturnT, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

template <class Closure>
static std::unique_ptr<ClosureTask<void, Closure>> NewTask(Closure&& closure) {
    std::unique_ptr<ClosureTask<void, Closure>> ptr(new ClosureTask<void, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

template <class ReturnT,class Closure>
static std::shared_ptr<ClosureTask<ReturnT, Closure>> MakeSharedTask(Closure&& closure) {
    std::shared_ptr<ClosureTask<ReturnT, Closure>> ptr(new ClosureTask<ReturnT, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

template <class Closure>
static std::shared_ptr<ClosureTask<void, Closure>> MakeSharedTask(Closure&& closure) {
    std::shared_ptr<ClosureTask<void, Closure>> ptr(new ClosureTask<void, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

/*
 ** Task Queue
 */
class TaskQueue {
public:
    static void SleepMs(uint64_t ms){
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    
    static int64_t NowMs() {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
    
    TaskQueue(): flush_(false){
        std::thread t(std::bind(&TaskQueue::run, this));
        thread_ = std::move(t);
    }
    
    TaskQueue(const TaskQueue&) = delete;
    
    TaskQueue operator=(const TaskQueue&) = delete;
    
    ~TaskQueue(){
        if(thread_.joinable()){
            flush_ = true;
            {
                std::unique_lock<std::mutex> guard(mutex_);
                
                // wake up task queue thread
                task_list_.push_back(nullptr);
                
                cond_.notify_one();
            }
            
            thread_.join();
        }
    }
    
    // is current thread == task_queue thread?
    bool is_current() const {
        return thread_.get_id() == std::this_thread::get_id();
    }
    
    // post an task to task_queue thread
    // async
    template <class Closure>
    bool post(Closure&& closure) {
        return post_delayed(closure,0);
    }
    
    template <class Closure>
    bool post_delayed(Closure&& closure, uint32_t delay_ms) {
        std::unique_lock<std::mutex> guard(mutex_);
        
        auto task = MakeSharedTask<void>(std::forward<Closure>(closure));
        
        if(delay_ms > 0){
            task->invoke_ms = NowMs() + delay_ms;
        }
        
        task_list_.push_back(std::move(task));
        cond_.notify_one();
        return true;
    }
    
    // invoke an task
    // sync
    template <class ReturnT, class Closure>
    ReturnT invoke(Closure&& closure) {
        
        auto task = MakeSharedTask<ReturnT, Closure>(std::forward<Closure>(closure));
        
        task->is_sync = true;
        
        {
            std::unique_lock<std::mutex> guard(mutex_);
            
            task_list_.push_back(task);
            
            cond_.notify_one();
        }
        
        {
            std::unique_lock<std::mutex> guard(sync_mutex_);
            
            sync_cond_.wait(guard, [&] { return task->finished; });
        }
        
        return task->move_result();
    }
    
private:
    void run() {
        while (true) {
            std::shared_ptr<Task> task;
            bool found_task = false;
            
            {
                std::unique_lock<std::mutex> guard(mutex_);
                cond_.wait(guard, [&] { return !task_list_.empty(); });
                
                if (task_list_.empty()) {
                    continue;
                }
                
                for(auto it = task_list_.begin(); it != task_list_.end(); ++it){
                    
                    task = *it;
                    
                    if(task == nullptr){
                        break;
                    }
                    
                    if(flush_ || task->invoke_ms <= NowMs()){
                        found_task = true;
                        task_list_.erase(it);
                        break;
                    }
                }
            }
            
            if (task == nullptr) {
                break;
            }
            
            if(!found_task){
                SleepMs(1);
                continue;
            }
            
            task->run();
            
            if(task->is_sync) {
                std::unique_lock<std::mutex> guard(sync_mutex_);
                
                task->finished = true;
                
                sync_cond_.notify_all();
            }
        }
    }
    
    std::mutex mutex_;
    std::condition_variable cond_;
    
    std::thread thread_;
    
    std::deque<std::shared_ptr<Task> > task_list_;
    
    std::mutex sync_mutex_;
    std::condition_variable sync_cond_;
    
    std::atomic<bool> flush_;
};

/*
 void test(){
 
 TaskQueue task_queue;
 
 int value = 0;
 
 int n = task_queue.invoke<int>([&]{
    printf("test\n");
    value += 10;
    return 1;
 });
 
 assert(n == 1);
 assert(value == 10);
 
 std::string str = task_queue.invoke<int>([&]{
    return "123";
 });
 
 assert(str == "123");
 
 task_queue.post([&]{
    printf("test2\n");
 });
 
 task_queue.post_delayed([&]{
    printf("test3\n");
 }, 1000);
 
 }
 */

}

#endif // _TASK_QUEUE_H_2024_
