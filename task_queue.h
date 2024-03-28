/*
*  Created by mobinsheng.
*/
#ifndef _LAZY_TASK_QUEUE_H_
#define _LAZY_TASK_QUEUE_H_
#pragma once
#include <assert.h>
#include <stdint.h>
#include <mutex>
#include <deque>
#include <thread>
#include <condition_variable>
#include <memory>
#include <type_traits>
#include <utility>
#include <functional>
#include <atomic>
#include <map>

#include "time_utils.h"

namespace lazy {

class QueuedTask {
public:
    QueuedTask() {}
    virtual ~QueuedTask() {}
    virtual void run() = 0;

    // 重载函数调用运算符,直接调用run
    void operator()() { run(); }
    
    // 唯一标识
    uint64_t id = static_cast<uint64_t>(-1);
    
    // is sync task
    // 是否为同步任务
    bool is_sync = false;

    // is task finish
    // 任务是否执行结束
    bool finished = false;
    
    // 异步任务 -- begin
    
    // 投递到任务队列的时刻
    int64_t post_time_ms = 0;
    
    // 延迟执行的时间
    uint32_t delay_ms = 0;
    
    // 是否循环执行/周期执行（定时器），异步任务使用
    bool loop = false;
    
    // 异步任务 -- end
private:
    QueuedTask(const QueuedTask&) = delete;
};

template <class ReturnT, class Closure>
class ClosureTask : public QueuedTask {
public:
    explicit ClosureTask(Closure&& closure) : closure_(std::forward<Closure>(closure)) {}

    virtual void run() override {
        result_ = closure_();
    }

    ReturnT move_result() { return std::move(result_); }

private:
    Closure closure_;
    ReturnT result_;
};

template <class Closure>
class ClosureTask<void, Closure> : public QueuedTask {
public:
    explicit ClosureTask(Closure&& closure) : closure_(std::forward<Closure>(closure)) {}

    virtual void run() override {
        closure_();
    }

    void move_result() { }

private:
    Closure closure_;
};


template <class ReturnT, class Closure>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure) {
    std::unique_ptr<QueuedTask> ptr(new ClosureTask<ReturnT, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

template <class Closure>
static std::unique_ptr<QueuedTask> NewClosure(Closure&& closure) {
    std::unique_ptr<QueuedTask> ptr(new ClosureTask<void, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

typedef std::shared_ptr<QueuedTask> SharedClosure;

template <class ReturnT, class Closure>
static SharedClosure MakeSharedClosure(Closure&& closure) {
    SharedClosure ptr(new ClosureTask<ReturnT, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

template <class Closure>
static SharedClosure MakeSharedClosure(Closure&& closure) {
    SharedClosure ptr(new ClosureTask<void, Closure>(std::forward<Closure>(closure)));
    return ptr;
}

/*
** 任务队列
*/
class TaskQueue {
public:
    TaskQueue() {}

    ~TaskQueue() {
        if (thread_.joinable()) {
            {
                std::unique_lock<std::mutex> guard(mutex_);

                flush_ = true;

                //task_list_.push_back(nullptr);
                delayed_task_map_.insert(std::make_pair(TimeUtil::NowMs(), nullptr));

                cond_.notify_one();
            }
            thread_.join();
        }

    }

    void start() {
        maybe_create_thread();
    }

    void stop() {

    }
    
    // 判断当前所在的线程是否和任务队列的线程为同一个
    bool is_current() {
        return std::this_thread::get_id() == thread_.get_id();
    }

    void set_name(const std::string& name) {
        std::unique_lock<std::mutex> guard(mutex_);
        name_ = name;
    }

    const std::string& name() const {
        return name_;
    }
    
    // 添加异步任务，和post效果一样
    template <class Closure>
    void add_task(Closure&& closure, uint64_t id = INVALID_ID) {
        post_delayed(std::forward<Closure>(closure), 0);
    }
    
    // 添加异步任务，和add_task效果一样
    template <class Closure>
    void post(Closure&& closure, uint64_t id = INVALID_ID) {
        post_delayed(std::forward<Closure>(closure), 0);
    }
    
    // 添加带延迟的异步任务
    template <class Closure>
    void post_delayed(Closure&& closure, uint32_t delay_ms, uint64_t id = INVALID_ID) {
        maybe_create_thread();

        std::unique_lock<std::mutex> guard(mutex_);
        std::shared_ptr<QueuedTask> task = MakeSharedClosure<void, Closure>(std::forward<Closure>(closure));
        task->finished = false;
        task->id = id;
        task->post_time_ms = TimeUtil::NowMs();
        task->delay_ms = delay_ms;
        task->is_sync = false;
        task->loop = false;
        
        int64_t target_time_ms = task->post_time_ms + task->delay_ms;
        
        delayed_task_map_.insert(std::make_pair(target_time_ms , std::move(task)));
        cond_.notify_one();
    }
    
    // 添加定时器，需要明确指定一个id
    template <class Closure>
    bool add_timer(Closure&& closure, uint32_t interval_ms, uint64_t id) {
        
        if(interval_ms == 0 || id == INVALID_ID){
            return false;
        }
        
        maybe_create_thread();
        
        std::unique_lock<std::mutex> guard(mutex_);
        std::shared_ptr<QueuedTask> task = MakeSharedClosure<void, Closure>(std::forward<Closure>(closure));
        task->finished = false;
        task->id = id;
        task->post_time_ms = TimeUtil::NowMs();
        task->delay_ms = interval_ms;
        task->is_sync = false;
        task->loop = true;
        
        int64_t target_time_ms = task->post_time_ms + task->delay_ms;
        
        delayed_task_map_.insert(std::make_pair(target_time_ms, std::move(task)));
        cond_.notify_one();
        
        return true;
    }
    
    // 移除定时器
    void remove_timer(uint64_t id) {
        std::unique_lock<std::mutex> guard(mutex_);
        
        if(id == INVALID_ID){
            return;
        }
        
        for(auto it = delayed_task_map_.begin(); it != delayed_task_map_.end();){
            if(it->second == nullptr){
                ++it;
                continue;
            }
            
            if(it->second->id == id){
                assert(it->second->loop == true);
                it = delayed_task_map_.erase(it);
                break;
            }
            else {
                ++it;
            }
        }
    }

    // 执行同步任务
    template <class ReturnT, class Closure>
    ReturnT invoke(Closure&& closure) {
        maybe_create_thread();

        std::shared_ptr<QueuedTask> task = MakeSharedClosure<ReturnT, Closure>(std::forward<Closure>(closure));
        task->finished = false;
        task->id = INVALID_ID;
        task->post_time_ms = 0;
        task->delay_ms = 0;
        task->is_sync = true;
        task->loop = false;
        
        {
            std::unique_lock<std::mutex> guard(mutex_);
            task_list_.push_back(task);
            cond_.notify_one();
        }
        {
            // 等待任务执行结束
            std::unique_lock<std::mutex> guard(sync_mutex_);
            sync_cond_.wait(guard, [&] {
                return task->finished;
            });
        }
        
        // 返回结果
        ClosureTask<ReturnT, Closure>* ptr = (ClosureTask<ReturnT, Closure>*)task.get();
        return ptr->move_result();
    }
private:
    
    // 创建任务队列线程
    void maybe_create_thread() {

        if (thread_.joinable()) {
            return;
        }

        std::unique_lock<std::mutex> guard(mutex_);

        if (thread_.joinable()) {
            return;
        }

        thread_ = std::thread(std::bind(&TaskQueue::run, this));
    }
    
    // 任务队列线程函数
    void run() {
        while (true) {
            std::shared_ptr<QueuedTask> task;

            bool sleep = false;

            bool exit_loop = false;

            do
            {
                std::unique_lock<std::mutex> guard(mutex_);
                cond_.wait(guard, [&] {
                    return (!task_list_.empty()) || (!delayed_task_map_.empty());
                });

                if (task_list_.empty() && delayed_task_map_.empty()) {
                    continue;
                }

                int64_t now_ms = TimeUtil::NowMs();
                
                // 把超时的任务从延迟队列中移动到任务队列
                for (auto it = delayed_task_map_.begin(); it != delayed_task_map_.end();) {
                    if (it->first <= now_ms || flush_) {
                        task_list_.push_back(std::move(it->second));
                        it = delayed_task_map_.erase(it);
                    }
                    else {
                        break;
                    }
                }

                if (task_list_.empty()) {
                    if (delayed_task_map_.empty()) {
                        // just wait
                        // 两个队列都是空，那么继续等待
                        continue;
                    }
                    else {
                        // wait delayed task timeout
                        // 延迟队列不为空（还有任务没有超时），那么需要sleep一下
                        sleep = true;

                    }
                }
                else {
                    task = std::move(task_list_.front());
                    task_list_.pop_front();
                    
                    // task等于null是退出的标识
                    if (task == nullptr) {
                        exit_loop = true;
                    }
                }

            } while (0);
            
            // 退出
            if (exit_loop) {
                break;
            }
            
            // 休眠等待任务超时
            if (sleep) {
                // std::this_thread::yield();
                // 注意不能休眠太久，要不然有新的任务时，反应不过来
                // 也可以用std::this_thread::yield，但是yield虽然能让线程切换，但是cpu占有率还是太高了
                // 这里就使用sleep来代替
                TimeUtil::SleepUs(500);
                continue;
            }

            assert(task != nullptr);

            task->run();
            
            // 对于同步任务，在这里进行唤醒操作
            if (task->is_sync) {
                std::unique_lock<std::mutex> guard(sync_mutex_);
                task->finished = true;
                sync_cond_.notify_all();
            }
            
            if(!flush_ && task->loop && task->delay_ms > 0){
                task->post_time_ms = TimeUtil::NowMs();
                
                uint64_t target_time_ms = task->post_time_ms + task->delay_ms;
                
                std::unique_lock<std::mutex> guard(mutex_);
                
                delayed_task_map_.insert(std::make_pair(target_time_ms, std::move(task)));
            }
        }
    }
    
    const static uint64_t INVALID_ID= static_cast<uint64_t>(-1);
    

    std::mutex mutex_;
    std::condition_variable cond_;

    std::thread thread_;

    std::deque<std::shared_ptr<QueuedTask>> task_list_;

    std::mutex sync_mutex_;
    std::condition_variable sync_cond_;

    std::multimap<uint64_t/*time ms*/, std::shared_ptr<QueuedTask>> delayed_task_map_;

    bool flush_ = false;

    std::string name_ = "";
};

}


#endif // _LAZY_TASK_QUEUE_H_
