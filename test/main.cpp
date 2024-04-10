//
//  main.cpp
//  lazy_base
//

#include <iostream>
#include <future>
#include "task_queue.h"
#include <list>
#include "test_invoke.h"
#include "test_post.h"
#include "test_timer.h"
#include "test_repeat_task.h"
#include "test_event.h"
#include "test_data_queue.h"
#include "time_utils.h"
#include "test_log.h"
#include "test_global_config.h"
#include "test_random.h"

#include <tuple>
#include <functional>

static auto func = [](){
    std::string str = "";
    for(int i = 0; i < 4; ++i){
        str += std::to_string(i);
    }
    
    if(str.length() > 2){
        return true;
    }
    else {
        return false;
    }
};

static void test_std(){
    int64_t sum_cost = 0;
    
    
    for(int index = 0; index < 10; ++index)
    {
        std::future<bool>* rets = new std::future<bool>[1024000];
        
        int64_t t1 = lazy::TimeUtil::NowMs();
        {
            for(int i = 0; i < 1024000; ++i){
                rets[i] = std::async(func);
            }
            
            for(int i = 0; i < 1024000; ++i){
                rets[i].get();
            }
        }
        int64_t t2 = lazy::TimeUtil::NowMs();
        
        
        delete [] rets;
        
        std::cout << "cost " << t2 - t1 << " ms" << std::endl;
        
        sum_cost += (t2 - t1);
    }
    
    std::cout << "avg_cost " << sum_cost / 10 << " ms" << std::endl;
}

static void test_tq(){
    int64_t sum_cost = 0;
    for(int index = 0; index < 10; ++index)
    {
        
        int64_t t1 = lazy::TimeUtil::NowMs();
        {
            lazy::TaskQueue task_queue;
            
            for(int i = 0; i < 1024000; ++i){
                task_queue.invoke<bool>(func);
            }
        }
        int64_t t2 = lazy::TimeUtil::NowMs();
        
        std::cout << "cost " << t2 - t1 << " ms" << std::endl;
        
        sum_cost += (t2 - t1);
    }
    
    std::cout << "avg_cost " << sum_cost / 10 << " ms" << std::endl;
}

 int test_add(int a, int b, int c) {
    return a + b + c;
}


static void test_aync(){
    lazy::TaskQueue task_queue;
    {
        // 测试异步任务返回值
        auto func = [&](){
            return 10;
        };
        
        std::packaged_task<int()> packaged(func);
        
        std::future<int> result = packaged.get_future();
        
        
        task_queue.post(std::move(packaged));
        
        int ret = result.get();
        assert(ret == 10);
        
        std::cout << ret << std::endl;
    }
    
    {
        int a = 1,b = 2,c = 3;
        
        auto func2 = std::bind(&test_add, a, b ,c);
        
        // 因为func2已经绑定所有参数，因此函数签名变成int()
        std::packaged_task<int()> packaged(std::move(func2));
        
        std::future<int> result = packaged.get_future();
        
        task_queue.post(std::move(packaged));
        
        int ret = result.get();
        assert(ret == 6);
        
        std::cout << ret << std::endl;
    }
}

int main(int argc, const char * argv[]) {
    
    /*TestInvoke();
    
    TestPost();
    
    TestTimer();
    
    TestRepeat();
    
    //TestEvent();
    
    //TestDataQueue();
    
    Testlog();
    
    TestGlobalConfig();
    
    TestRandom();*/
    
    //TestRingbuffer();
    
    
    /*bool use_std = false;
    
    if(use_std){
        test_std();
    }
    else{
        test_tq();
    }*/
    
    test_aync();
    
    return 0;
}

