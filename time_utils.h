//
//  time_utils.h
//

#ifndef __LAZY_TIME_UTILS_H_2024__
#define __LAZY_TIME_UTILS_H_2024__

#include <chrono>
#include <thread>

#include "lazy_base_common.h"

// Time Utils

namespace lazy {

class TimeUtil {
public:
    // milliseconds
    static void SleepMs(uint64_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    
    // microseconds
    static void SleepUs(uint64_t us) {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
    
    // nanoseconds
    static void SleepNs(uint64_t ns) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }
    
    // milliseconds
    static int64_t NowMs() {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
    
    // microseconds
    static int64_t NowUs() {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
    
    // nanoseconds
    static int64_t NowNs() {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }
};

}

#endif /* __LAZY_TIME_UTILS_H_2024__ */
