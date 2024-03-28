//
//  test_invoke.h
//

#ifndef test_invoke_h
#define test_invoke_h

#include "task_queue.h"
#include <stdio.h>


void TestInvoke(){
    class FileWriter {
    public:
        FileWriter(){
            task_queue_.start();
        }
        
        ~FileWriter(){
            task_queue_.stop();
        }
        
        bool Open(const std::string& name){
            // 判断当前所在线程和任务队列的线程是否为同一个
            if(!task_queue_.is_current()){
                return task_queue_.invoke<bool>([&]{
                    return Open(name);
                });
            }
            
            if(fp_ != nullptr){
                return true;
            }
            
            fp_ = fopen(name.c_str(), "wb");
            
            if(!fp_){
                return false;
            }
            
            return true;
        }
        
        bool Write(const uint8_t* data, size_t size){
            // 判断当前所在线程和任务队列的线程是否为同一个
            if(!task_queue_.is_current()){
                return task_queue_.invoke<bool>([&]{
                    return Write(data, size);
                });
            }
            
            if(data == nullptr || size == 0){
                return false;
            }
            
            if(fp_ == nullptr){
                return false;
            }
            
            size_t ret = fwrite(data, 1, size, fp_);
            
            if(ret != size){
                return false;
            }
            
            return true;
        }
        
        void Flush(){
            // 判断当前所在线程和任务队列的线程是否为同一个
            if(!task_queue_.is_current()){
                return task_queue_.invoke<void>([&]{
                    return Flush();
                });
            }
            
            if(!fp_){
                return;
            }
            
            fflush(fp_);
        }
        
        void Close(){
            // 判断当前所在线程和任务队列的线程是否为同一个
            if(!task_queue_.is_current()){
                return task_queue_.invoke<void>([&]{
                    return Close();
                });
            }
            
            if(!fp_){
                return ;
            }
            
            Flush();
            
            fclose(fp_);
            
            fp_ = nullptr;
        }
        
    private:
        lazy::TaskQueue task_queue_;
        FILE* fp_ = nullptr;
    };
    
    std::string file_name = "1.log";
    
    FileWriter writer;
    
    std::thread t1([&]{
        
        lazy::TimeUtil::SleepMs(1000);
        
        writer.Open(file_name);
        
        std::string text = "thread1 write line!\n";
        writer.Write((const uint8_t*)text.data(), text.size() + 1);
        
        writer.Flush();
    });
    
    std::thread t2([&]{
        writer.Open(file_name);
        
        lazy::TimeUtil::SleepMs(1000);
        
        std::string text = "thread2 write line!\n";
        writer.Write((const uint8_t*)text.data(), text.size() + 1);
        
        writer.Flush();
    });
    
    std::thread t3([&]{
        writer.Open(file_name);
        
        std::string text = "thread3 write line!\n";
        writer.Write((const uint8_t*)text.data(), text.size() + 1);
        
        lazy::TimeUtil::SleepMs(1000);
        
        writer.Flush();
    });
    
    std::thread t4([&]{
        writer.Open(file_name);
        
        std::string text = "thread4 write line!\n";
        writer.Write((const uint8_t*)text.data(), text.size() + 1);
        
        writer.Flush();
        
        lazy::TimeUtil::SleepMs(1000);
    });
    
    std::thread t5([&]{
        writer.Open(file_name);
        
        std::string text = "thread5 write line!\n";
        writer.Write((const uint8_t*)text.data(), text.size() + 1);
        
        writer.Flush();
    });
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    
    std::thread t6([&]{
        writer.Close();
    });
    
    std::thread t7([&]{
        writer.Close();
    });
    
    t6.join();
    t7.join();

}

#endif /* test_invoke_h */
