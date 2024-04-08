//
//  test_global_config.h
//  lazy_base
//
//  Created by 莫斌生 on 2024/3/29.
//

#ifndef test_global_config_h
#define test_global_config_h

#include "global_config.h"
#include <assert.h>

void TestGlobalConfig(){
    using namespace lazy;
    
    for(uint8_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(int8_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(uint16_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(int16_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(uint32_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(int32_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(uint64_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(int64_t i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(unsigned int i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(int i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        GlobalConfig::instance().set(key0, i);
        
        assert(GlobalConfig::instance().get(key0, 0) == i);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
    
    for(int i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        bool val = (i % 2) == 0;
        
        GlobalConfig::instance().set(key0, val);
        
        assert(GlobalConfig::instance().get(key0, false) == val);
        
        assert(GlobalConfig::instance().get(key1, false) == false);
    }
    
    for(int i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        std::string val = key0;
        
        GlobalConfig::instance().set(key0, val);
        
        assert(GlobalConfig::instance().get(key0, "") == val);
        
        assert(GlobalConfig::instance().get(key1, "") == "");
    }
    
    for(int i = 0; i < 100; i += 2){
        std::string key0 = std::to_string(i);
        std::string key1 = std::to_string(i+1);
        
        double val = i * 1.0;
        
        GlobalConfig::instance().set(key0, val);
        
        assert(GlobalConfig::instance().get(key0, 0) == val);
        
        assert(GlobalConfig::instance().get(key1, 0) == 0);
    }
}

#endif /* test_global_config_h */
