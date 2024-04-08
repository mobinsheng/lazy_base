//
//  global_config.h
//

#ifndef _LAZY_GLOBAL_CONFIG_H_
#define _LAZY_GLOBAL_CONFIG_H_

#include "lazy_base_common.h"

#include <unordered_map>
#include <string>
#include <mutex>

namespace lazy {

class GlobalConfig {
public:
    static GlobalConfig& instance(){
        static GlobalConfig inst;
        return inst;
    }
    
    GlobalConfig(){
        
    }
    
    ~GlobalConfig(){
        
    }
    
    void set(const std::string& key, uint64_t val){
        std::unique_lock<std::mutex> lock(mutex_);
        uint64_map_[key] = val;
    }
    
    void set(const std::string& key, int64_t val){
        std::unique_lock<std::mutex> lock(mutex_);
        int64_map_[key] = val;
    }
    
    void set(const std::string& key, unsigned int val){
        std::unique_lock<std::mutex> lock(mutex_);
        uint64_map_[key] = val;
    }
    
    void set(const std::string& key, int val){
        std::unique_lock<std::mutex> lock(mutex_);
        int64_map_[key] = val;
    }
    
    void set(const std::string& key, bool val){
        std::unique_lock<std::mutex> lock(mutex_);
        bool_map_[key] = val;
    }
    
    void set(const std::string& key, const std::string& val){
        std::unique_lock<std::mutex> lock(mutex_);
        str_map_[key] = val;
    }
    
    void set(const std::string& key, const double& val){
        std::unique_lock<std::mutex> lock(mutex_);
        float_map_[key] = val;
    }
    
    uint64_t get(const std::string& key, uint64_t default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &uint64_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return it->second;
    }
    
    int get(const std::string& key, int default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &int64_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return (int)it->second;
    }
    
    unsigned int get(const std::string& key, unsigned int default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &uint64_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return (unsigned int)it->second;
    }
    
    int64_t get(const std::string& key, int64_t default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &int64_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return it->second;
    }
    
    bool get(const std::string& key, bool default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &bool_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return it->second;
    }
    
    const std::string& get(const std::string& key, const std::string& default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &str_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return it->second;
    }
    
    const char* get(const std::string& key, const char* default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &str_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return it->second.c_str();
    }
    
    double get(const std::string& key, double default_val){
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto* m = &float_map_;
        
        auto it = m->find(key);
        
        if(it == m->end()){
            return default_val;
        }
        
        return it->second;
    }
private:
    
    LAZY_DISALLOW_COPY_AND_ASSIGN(GlobalConfig);
    
    std::mutex mutex_;
    
    std::unordered_map<std::string, uint64_t> uint64_map_;
    std::unordered_map<std::string, int64_t> int64_map_;
    
    std::unordered_map<std::string, bool> bool_map_;
    std::unordered_map<std::string, std::string> str_map_;
    
    std::unordered_map<std::string, double> float_map_;
};

}

#endif /* _LAZY_GLOBAL_CONFIG_H_ */
