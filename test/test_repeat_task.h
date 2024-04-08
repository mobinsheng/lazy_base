//
//  test_repeat_task.h
//  lazy_base
//
//  Created by 莫斌生 on 2024/4/1.
//

#ifndef test_repeat_task_h
#define test_repeat_task_h

#include "task_queue.h"

void TestRepeat(){
    int repeat_num = 20;
    int interval_ms = 500;
    
    uint64_t task_id = 0;
    
    int invoke_count = 0;
    
    lazy::TaskQueue task_queue;
    
    task_queue.post_delayed_and_repeat([&]{
        
        ++invoke_count;
        
        printf("invoke count: %d\n", invoke_count);
        
    }, interval_ms, task_id, repeat_num);
    
    lazy::TimeUtil::SleepMs(11000);
    
    task_queue.stop();
}

#endif /* test_repeat_task_h */
