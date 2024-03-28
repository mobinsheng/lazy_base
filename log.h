//
//  log.h
//

#ifndef __LAZY_LOG_H_2024__
#define __LAZY_LOG_H_2024__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "data_queue.h"
#include "task_queue.h"

namespace lazy {

enum LOG_LEVEL {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
};


class Log {
public:
    struct Config {
        // 文件目录
        std::string dir = "./log/";
        // 文件名的前缀，最终的名字类似于 /dir/prefix_name_2024-11-03_20.30.14.log
        std::string prefix_name = "lazy_log";
        // 日志级别
        LOG_LEVEL level = LOG_LEVEL_INFO;
        // 文件最大的大小，超过就会重新创建一个文件写入
        size_t max_size_bytes = 1024 * 1024 * 8;
        // 刷新到磁盘的时间间隔
        uint32_t flush_interval_ms = 1000;
    };
    
    static Log& instance(){
        static Log inst;
        return inst;
    }
    
    bool init(const Config& conf){
        std::unique_lock<std::mutex> lock(mutex_);
        
        config_ = conf;
        
        // 内部自带锁
        task_queue_.start();
        
        // 内部自带锁
        log_cache_.start();
        
        task_queue_.add_timer([&]{
            write();
        }, conf.flush_interval_ms, 1);
        
        return true;
    }
    
    void uninit(){
        std::unique_lock<std::mutex> lock(mutex_);
        
        log_cache_.stop();
        
        task_queue_.remove_timer(1);
        
        task_queue_.post([&]{
            write();
            close();
        });
        
        task_queue_.stop();
    }
    
    void log(LOG_LEVEL log_level, const std::string& file, int line, const char *format, ...){
        
        if(log_level < config_.level){
            return;
        }
        
        char buffer[4096];
        
        va_list args;

        va_start(args, format);
        
        snprintf(buffer, 4096, format, args);
        
        va_end(args);
        
        std::string full_str = "[" + geneate_log_ctx_prefix() + "] "
        + "(" + etract_file_name(file.c_str()) +  ":" + std::to_string(line) + ") : "
        + "[" + level_to_string(log_level) + "] "
        + buffer + line_end_;
        
        // 内部自带锁
        log_cache_.push_back(full_str);
    }
    
private:
    // 根据时间信息生成文件名
    static std::string generate_name(){
        // 获取当前时间点
        auto now = std::chrono::system_clock::now();
            
        // 转换为time_t，以便于转换为本地时间
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            
        // 创建一个tm结构体来保存转换后的时间
        std::tm now_tm = *std::localtime(&now_time_t);
            
        // 使用stringstream和put_time来格式化时间
        std::stringstream ss;
        ss << std::put_time(&now_tm, "%Y-%m-%d %H_%M_%S"); // 格式化为YYYY-MM-DD HH:MM:SS格式
            
        // 返回格式化后的字符串
        return ss.str();
    }
    
    // 根据时间信息生成日志前缀
    static std::string geneate_log_ctx_prefix(){
        // 获取当前时间点
        auto now = std::chrono::system_clock::now();
            
        // 转换为time_t，以便于转换为本地时间
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            
        // 创建一个tm结构体来保存转换后的时间
        std::tm now_tm = *std::localtime(&now_time_t);
        
        int ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
            
        // 使用stringstream和put_time来格式化时间
        std::stringstream ss;
        ss << std::put_time(&now_tm, "%Y%m%d %H:%M:%S.") << ms ; // 格式化为YYYY-MM-DD HH:MM:SS格式
            
        // 返回格式化后的字符串
        return ss.str();
    }
    
    // 写入
    void write(){
        
        while(true){
            
            if(!maybe_open()){
                break;
            }
            
            if(!fp_){
                break;
            }
            
            std::string log;
            
            if(!log_cache_.pop_front(log)){
                break;
            }
            
            size_t ret = fwrite(log.c_str(), 1, log.size(), fp_);
            
            if(ret != log.size()){
                break;
            }
            
            file_size_bytes_ += log.size();
        }
        
        if(fp_){
            fflush(fp_);
        }
    }
    
    bool maybe_open(){
        // 创建新的文件
        if(fp_ == nullptr || file_size_bytes_ >= config_.max_size_bytes){
            
            close();
            
            file_size_bytes_ = 0;
            
            std::string name = config_.dir + "/" + config_.prefix_name;
            name += "_";
            name += generate_name();
            name += ".log";
            
            fp_ = fopen(name.c_str(), "wb");
        }
        
        if(!fp_){
            return false;
        }
        
        return true;
    }
    void close(){
        if(fp_){
            fflush(fp_);
            fclose(fp_);
            fp_ = nullptr;
        }
    }
    
    // 获取__FILE__里面不带路径的文件名
    const char* etract_file_name(const char* path) {
        const char* file = strrchr(path, '/'); // 对Unix/Linux
        if (file)
            return file + 1;
        file = strrchr(path, '\\'); // 对Windows
        if (file)
            return file + 1;
        return path; // 如果都没有找到，返回原始的路径
    }
    
    // level转字符串
    const char* level_to_string(LOG_LEVEL level){
        switch (level) {
            case LOG_LEVEL_DEBUG:
                return "DEBUG";
                break;
            case LOG_LEVEL_INFO:
                return "INFO";
                break;
            case LOG_LEVEL_WARN:
                return "WARN";
                break;
            case LOG_LEVEL_ERROR:
                return "ERROR";
                break;
            default:
                return "";
                break;
        }
    }
private:
    
    std::mutex mutex_;
    
    std::condition_variable cond_;
    
    Config config_;
    
    DataQueue<std::string> log_cache_;
    
    TaskQueue task_queue_;
    
    FILE* fp_ = nullptr;
    
    std::string line_end_ = "\r\n";
    
    size_t file_size_bytes_ = 0;
};

#define LogWarn(format, ...) lazy::Log::instance().log(lazy::LOG_LEVEL_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LogInfo(format, ...) lazy::Log::instance().log(lazy::LOG_LEVEL_INFO,  __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LogError(format, ...) lazy::Log::instance().log(lazy::LOG_LEVEL_ERROR,  __FILE__, __LINE__,format, ##__VA_ARGS__)
#define LogDebug(format, ...) lazy::Log::instance().log(lazy::LOG_LEVEL_DEBUG,  __FILE__, __LINE__,format, ##__VA_ARGS__)


}
#endif /* __LAZY_LOG_H_2024__ */
