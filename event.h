//
//  event.h
//

#ifndef _LAZY_EVENT_H_2024_
#define _LAZY_EVENT_H_2024_

#include <mutex>
#include <condition_variable>
#include <chrono>

namespace lazy {

/**
 * 所谓假唤醒，有两种说法（场景：一个生产者、多个消费者）：
 * 1、生产者产生一个数据，使用notify_all把所有的消费者唤醒，但是队列中只有一个数据，只有一个消费者能取出数据，对于其他消费者来说，这就是假唤醒
 * 2、由于系统的调度，阻塞的消费者线程被临时唤醒
 * 3、解决假唤醒的方法是，在被唤醒之后，检查某个标识是否被设置，如果被设置了，那么表示真的被唤醒，否则是假唤醒
 */
class Event {
public:
    Event(){
        
    }
    
    ~Event(){
    }
    
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    
    bool wait(int32_t timeout_ms = -1){
        std::unique_lock<std::mutex> lock(mutex_);
        
        if(timeout_ms > 0){
            cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&]{return state_ == STATE_WAKEUP;});
        }
        else {
            cond_.wait(lock,[&]{return (state_ == STATE_WAKEUP);});
        }
        
        state_ = STATE_WAIT;
        
        return true;
    }
    
    void wake_up(){
        std::unique_lock<std::mutex> lock(mutex_);
        
        state_ = STATE_WAKEUP;
        
        cond_.notify_all();
    }
    
private:
    enum State{
        STATE_WAIT = 0,
        STATE_WAKEUP = 1,
    };
    
    std::mutex mutex_;
    std::condition_variable cond_;
    
    State state_ = STATE_WAIT;
};

}

#endif /* _LAZY_EVENT_H_2024_ */
