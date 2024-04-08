//
//  test_event.h
//

#ifndef test_event_h
#define test_event_h

#include "event.h"

void TestEvent(){
    std::deque<int64_t> data_queue;
    
    lazy::Event event;
    
    bool stop = false;
    
    std::atomic<int> running_consumers (0);
    
    std::thread producer([&]{
        for(int i = 0; i < 1024; ++i){
            data_queue.push_back(i);
            
            event.wake_up();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        printf("producer stoped \n");
        
        stop = true;
        
        // 唤醒一个消费者，让其结束
        event.wake_up();
    });
    
    std::thread consumer1([&]{
        ++running_consumers;
        
        while(!stop){
            event.wait();
            if(stop){
                break;
            }
            printf("consumer1 cost: %lld \n", data_queue.front());
            data_queue.pop_front();
        }
        printf("consumer1 stoped \n");
        --running_consumers;
        
        // 唤醒另一个消费者，让其结束
        if(running_consumers > 0){
            event.wake_up();
        }
    });
    
    std::thread consumer2([&]{
        ++running_consumers;
        while(!stop){
            event.wait();
            if(stop){
                break;
            }
            printf("consumer2 cost: %lld \n", data_queue.front());
            data_queue.pop_front();
        }
        printf("consumer2 stoped \n");
        
        --running_consumers;
        
        // 唤醒另一个消费者，让其结束
        if(running_consumers > 0){
            event.wake_up();
        }
    });
    
    std::thread consumer3([&]{
        ++running_consumers;
        while(!stop){
            event.wait();
            if(stop){
                break;
            }
            printf("consumer3 cost: %lld \n", data_queue.front());
            data_queue.pop_front();
        }
        printf("consumer3 stoped \n");
        
        --running_consumers;
        
        // 唤醒另一个消费者，让其结束
        if(running_consumers > 0){
            event.wake_up();
        }
    });
    
    std::thread consumer4([&]{
        ++running_consumers;
        
        while(!stop){
            event.wait();
            if(stop){
                break;
            }
            printf("consumer4 cost: %lld \n", data_queue.front());
            data_queue.pop_front();
        }
        printf("consumer4 stoped \n");
        
        --running_consumers;
        
        // 唤醒另一个消费者，让其结束
        if(running_consumers > 0){
            event.wake_up();
        }
    });
    
    
    producer.join();
    
    consumer1.join();
    
    consumer2.join();
    
    consumer3.join();
    
    consumer4.join();
    
    int64_t time_ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    
    event.wait(2000);
    
    int64_t time_ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    
    printf("wait time: %lld \n", time_ms2 - time_ms1);
}


#endif /* test_event_h */
