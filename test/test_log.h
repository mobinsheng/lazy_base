//
//  test_log.h
//

#ifndef test_log_h
#define test_log_h

#include "log.h"
#include "time_utils.h"
#include <mutex>
#include <thread>

static void external_log_func(lazy::LOG_LEVEL level, const char* msg){
    printf("%s",msg);
}

void Testlog(){
    bool use_external_log_func = false;
    //std::thread::id
    lazy::Logger::Config conf;
    
    conf.level = lazy::LOG_LEVEL_INFO;
    
    if(use_external_log_func){
        conf.log_cb = external_log_func;
    }
    else {
        conf.max_size_bytes = 1024*512;
        conf.flush_interval_ms = 1000;
    }
    
    lazy::Logger::instance().initialize(conf);
    
    std::thread t1([&]{
        for(int i = 0; i < 10240; ++i){
            std::string str = std::to_string(i);
            
            //log_info_periodic(1000, str.c_str());
            
            lazy::TimeUtil::SleepMs(1);
        }
    });
    
    std::thread t2([&]{
        for(int i = 0; i < 10240; ++i){
            std::string str = std::to_string(i);
            
            //LogInfo(str.c_str());
            //LogInfoPeriodic(1000, str.c_str());
            
            std::string sv = "sv";
            float fv = 1.0;
            double dv = 2.0;
            int iv = i;
            unsigned int uv = i;
            int8_t i8v = 0;
            uint8_t u8v = 0;
            int16_t i16v = 0;
            uint16_t u16v = 0;
            int32_t i32v = 0;
            uint32_t u32v = 0;
            uint64_t u64v = 0;
            int64_t i64v = 0;
            LogInfo << "number: " << i
            << " string: " << sv
            << " float: " << fv
            << " double: " << dv
            << " int: " << iv
            << " unsigned int: " << uv
            << " int8_t: " << i8v
            << " uint8_t: " << u8v
            << " int16_t: " << i16v
            << " uint16_t: " << u16v
            << " int32_t: " << i32v
            << " uint32_t: " << u32v
            << " uint64_t: " << u64v
            << " int64_t: " << i64v
            << " " << 0.123
            << " &dv: " << &dv;
            
            lazy::TimeUtil::SleepMs(1);
        }
    });
    
    std::thread t3([&]{
        for(int i = 0; i < 10240; ++i){
            std::string str = std::to_string(i);
            
            //log_error_periodic(1000, str.c_str());
            
            lazy::TimeUtil::SleepMs(1);
        }
    });
    
    t1.join();
    t2.join();
    t3.join();
    
    lazy::Logger::instance().uninitialize();
}


#endif /* test_log_h */
