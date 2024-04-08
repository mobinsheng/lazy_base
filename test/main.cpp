//
//  main.cpp
//  lazy_base
//

#include <iostream>
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
#include "test_ringbuffer.h"

static lazy::TaskQueue task_queue;

int main(int argc, const char * argv[]) {
    
    printf("is_current: %d\n", task_queue.is_current());
    
    /*TestInvoke();
    
    TestPost();
    
    TestTimer();
    
    TestRepeat();
    
    //TestEvent();
    
    //TestDataQueue();
    
    Testlog();
    
    TestGlobalConfig();
    
    TestRandom();*/
    
    TestRingbuffer();
    
    return 0;
}
