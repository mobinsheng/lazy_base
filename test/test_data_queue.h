//
//  test_data_queue.h
//

#ifndef test_data_queue_h
#define test_data_queue_h

#include "data_queue.h"

void TestDataQueue(){
    lazy::DataQueue<int64_t> data_queue;
    
    data_queue.start();
    
    std::thread producer([&]{
        for(int i = 0; i < 1024; ++i){
            data_queue.push_back(i);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        printf("producer stoped \n");
        
        data_queue.stop();
    });
    
    std::thread consumer1([&]{
        
        while(true){
            int64_t val = 0;
            if(!data_queue.pop_front(val)){
                break;
            }
            printf("consumer1 cost: %lld \n", val);
        }
        printf("consumer1 stoped \n");
        
    });
    
    std::thread consumer2([&]{
        while(true){
            int64_t val = 0;
            if(!data_queue.pop_front(val)){
                break;
            }
            printf("consumer2 cost: %lld \n", val);
        }
        printf("consumer2 stoped \n");
    });
    
    std::thread consumer3([&]{
        while(true){
            int64_t val = 0;
            if(!data_queue.pop_front(val)){
                break;
            }
            printf("consumer3 cost: %lld \n", val);
        }
        printf("consumer3 stoped \n");
    });
    
    std::thread consumer4([&]{
        
        while(true){
            int64_t val = 0;
            if(!data_queue.pop_front(val)){
                break;
            }
            printf("consumer4 cost: %lld \n", val);
        }
        printf("consumer4 stoped \n");
    });
    
    
    producer.join();
    
    consumer1.join();
    
    consumer2.join();
    
    consumer3.join();
    
    consumer4.join();
    
    
}


#endif /* test_data_queue_h */
