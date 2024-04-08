//
//  test_post.h
//

#ifndef test_post_h
#define test_post_h

#include "task_queue.h"
#include <stdio.h>

void TestPost(){
    class Sender {
    public:
        Sender(lazy::TaskQueue& tq,std::list<uint64_t>& dl): task_queue_(tq), data_list_(dl){}
        
        bool start(){
            return true;
        }
        
        // 发送数据
        void send(){
            return task_queue_.post([&]{
                data_list_.push_back(data_);
                this->data_++;
            });
        }
        
        // 停止发送
        void stop(){
            // 发送-1表示停止
            return task_queue_.post([&]{
                data_list_.push_back(-1);
            });
        }
        
    private:
        lazy::TaskQueue& task_queue_;
        std::list<uint64_t>& data_list_;
        
        uint64_t data_ = 0;
    };

    class Receiver {
    public:
        Receiver(lazy::TaskQueue& tq,std::list<uint64_t>& dl): task_queue_(tq), data_list_(dl){}
        
        bool start(){
            return true;
        }
        
        void receive(){
            
            task_queue_.post([&]{
                
                if(!data_list_.empty()){
                    uint64_t data = data_list_.front();
                    data_list_.pop_front();
                    
                    printf("recv data: %lld \n", data);
                    
                    if(data == -1){
                        // 结束
                        finish_ = true;
                        return;
                    }
                    
                    // 继续
                    task_queue_.post([&, this]{
                        receive();
                    });
                }
                else {
                    //  没有数据，等待10ms，继续处理
                    task_queue_.post_delayed([&, this]{
                        receive();
                    },10);
                }
            });
            
            
        }
        
        void stop(){
            
        }
        
        bool finished(){
            return finish_;
        }
        
    private:
        lazy::TaskQueue& task_queue_;
        std::list<uint64_t>& data_list_;
        bool finish_ = false;
    };
    
    lazy::TaskQueue task_queue;

    std::list<uint64_t> data_list;
    
    Sender sender(task_queue,data_list);
    
    Receiver receiver(task_queue,data_list);
    
    sender.start();
    receiver.start();
    
    std::thread t1([&]{
        for(int i = 0; i < 1024; ++i){
            sender.send();
            lazy::TimeUtil::SleepMs(10);
        }
        
        sender.stop();
    });
    
    std::thread t2([&]{
        
        receiver.receive();
        
        while(!receiver.finished()){
            lazy::TimeUtil::SleepMs(1);
        }
    });
    
    t1.join();
    t2.join();
}

#endif /* test_post_h */
