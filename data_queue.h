//
//  data_queue.h
//

#ifndef __LAZY_DATA_QUEUE_H__2024__
#define __LAZY_DATA_QUEUE_H__2024__

#include <deque>
#include <mutex>
#include <condition_variable>

namespace lazy {


template<class T>
class DataQueue {
public:
    
    bool push_back(const T& val) {
        std::unique_lock<std::mutex> lock(mutex_);
        if(stop_){
            return false;
        }
        queue_.push_back(val);
        cond_.notify_all();
        
        return true;
    }
    
    bool pop_front(T& val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        if(stop_){
            return false;
        }
        
        cond_.wait(lock, [&]{
            return (!queue_.empty() || stop_);
        });
        
        if(stop_){
            return false;
        }
        
        val = std::move(queue_.front());
        
        queue_.pop_front();
        
        return true;
    }
    
    bool front(T& val) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if(stop_){
            return false;
        }
        
        cond_.wait(lock, [&]{
            return (!queue_.empty() || stop_);
        });
        
        if(stop_){
            return false;
        }
        
        val = queue_.front();
        
        return true;
    }
    
    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    bool start(){
        std::unique_lock<std::mutex> lock(mutex_);
        
        stop_ = false;
        
        return true;
    }
    
    void stop(){
        std::unique_lock<std::mutex> lock(mutex_);
        
        stop_ = true;
        
        cond_.notify_all();
    }
    
private:
    std::mutex mutex_;
    std::condition_variable cond_;
    
    std::deque<T> queue_;
    bool stop_ = false;
};

}

#endif /* __LAZY_DATA_QUEUE_H__2024__ */
