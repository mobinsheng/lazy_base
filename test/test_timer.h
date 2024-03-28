//
//  test_timer.h
//

#ifndef test_timer_h
#define test_timer_h

#include "task_queue.h"
#include <stdio.h>

void TestTimer(){
    lazy::TaskQueue task_queue;
    
    task_queue.start();
    
    uint64_t timer_id1 = 123;
    uint64_t timer_id2 = 456;
    
    int64_t time_ms = lazy::TimeUtil::NowMs();
    
    task_queue.add_timer([&]{
        
        int64_t now_ms = lazy::TimeUtil::NowMs();
        
        printf("timer1 time_diff: %lld ms\n", now_ms - time_ms);
        
        time_ms = now_ms;
        
    }, 1000, timer_id1);
    
    task_queue.add_timer([&]{
        
        printf("timer2 \n");
        
    }, 666, timer_id2);
    
    lazy::TimeUtil::SleepMs(20000);
    
    task_queue.remove_timer(timer_id1);
    
    lazy::TimeUtil::SleepMs(5000);
    
    task_queue.remove_timer(timer_id2);
    
    task_queue.stop();
}

#endif /* test_timer_h */
