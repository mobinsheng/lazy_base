//
//  log.h
//

#ifndef __LAZY_LOG_H_2024__
#define __LAZY_LOG_H_2024__

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <chrono>
#include <deque>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "lazy_base_common.h"

#include "data_queue.h"
#include "task_queue.h"
#include "time_utils.h"

namespace lazy {

enum LOG_LEVEL {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
};

typedef void (*LogFuncCb)(LOG_LEVEL log_level, const char* utf8_data);

class LoggerHelper {
public:
    // 根据时间信息生成文件名
    static std::string generate_file_name(){
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
    
    // 把当前时间转换成字符串
    static std::string now_to_string() {
        char buf[64];

        auto now = std::chrono::system_clock::now();
        
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm now_tm = *std::localtime(&now_time_t);

        size_t written = strftime(buf, sizeof(buf), "%Y%m%d %H:%M:%S", &now_tm);
        
        if (written && (sizeof(buf) - written) > 5) {
            
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

            snprintf(buf + written, sizeof(buf) - written, ".%03u",
                static_cast<unsigned>(millis));
        }

        return std::move(buf);
    }
    
    // 获取__FILE__里面不带路径的文件名
    static const char* etract_file_name(const char* path) {
        const char* file = strrchr(path, '/'); // 对Unix/Linux
        if (file)
            return file + 1;
        file = strrchr(path, '\\'); // 对Windows
        if (file)
            return file + 1;
        return path; // 如果都没有找到，返回原始的路径
    }
    
    // level转字符串
    static const char* level_to_string(LOG_LEVEL level){
        switch (level) {
            case LOG_LEVEL_DEBUG:
                return "D";
                break;
            case LOG_LEVEL_INFO:
                return "I";
                break;
            case LOG_LEVEL_WARN:
                return "W";
                break;
            case LOG_LEVEL_ERROR:
                return "E";
                break;
            default:
                return "";
                break;
        }
    }
    
    static const char* get_thread_id(){
        static thread_local std::thread::id thread_id = std::this_thread::get_id();
        static thread_local std::string str;
        
        if(str.empty()){
            std::stringstream ss;
            ss << thread_id; // 格式化为
                
            // 返回格式化后的字符串
            str = ss.str();
        }
        
        return str.c_str();
    }
};

class Logger {
public:
    struct Config {
        // 日志级别
        LOG_LEVEL level = LOG_LEVEL_INFO;
        
        // 刷新的时间间隔
        uint32_t flush_interval_ms = 1000;
        
        // 日志回调
        LogFuncCb log_cb = nullptr;
        
        // 如果log_cb等于nullptr，那么下面的参数(dir、prefix_name、max_size_bytes)必须设置
        // 如果log_cb不等于nullptr，那么下面的参数(dir、prefix_name、max_size_bytes)可以不设置
        
        // 文件目录
        std::string dir = "./log/";
        
        // 文件名的前缀，最终的名字类似于 /dir/prefix_name_2024-11-03_20.30.14.log
        std::string prefix_name = "lazy_log";
        
        // 文件最大的大小，超过就会重新创建一个文件写入
        size_t max_size_bytes = 1024 * 1024 * 8;
    };
    
    static Logger& instance(){
        static Logger inst;
        return inst;
    }
    
    Logger(){
        inited_ = false;
    }
    
    ~Logger(){
        uninitialize();
    }
    
    bool initialize(const Config& conf){
        if(conf.dir.empty() || conf.prefix_name.empty() ||
           conf.max_size_bytes == 0 || conf.flush_interval_ms == 0){
            return false;
        }
        
        if(inited_){
            return true;
        }
        
        // 内部自带锁
        task_queue_.start();
        
        task_queue_.invoke<void>([&]{
            config_ = conf;
        });
        
        // 内部自带锁
        log_cache_.start();
        
        task_queue_.add_timer([&]{
            write();
        }, conf.flush_interval_ms, 1);
        
        inited_ = true;
        
        return true;
    }
    
    void uninitialize(){
        if(!inited_){
            return;
        }
        
        log_cache_.stop();
        
        task_queue_.remove_timer(1);
        
        task_queue_.post([&]{
            write();
            close();
        });
        
        task_queue_.stop();
        
        inited_ = false;
    }
    
    void log(LOG_LEVEL log_level, const std::string& file, int line, const char *format, ...){
        
        if(!inited_){
            return;
        }
        
        if(log_level < config_.level){
            return;
        }
        
        char buffer[4096];
        
        va_list args;

        va_start(args, format);
        
        snprintf(buffer,
                 4096,
                 format,
                 args);
        
        va_end(args);
        
        std::ostringstream print_stream;
        print_stream << "[";
        print_stream << LoggerHelper::now_to_string();
        print_stream << "] ";
        print_stream << "(";
        print_stream << LoggerHelper::etract_file_name(file.c_str());
        print_stream << ":";
        print_stream << line;
        print_stream << ") ";
        print_stream << ": ";
        print_stream << "[";
        print_stream << LoggerHelper::level_to_string(log_level);
        print_stream << "] ";
        print_stream << "[";
        print_stream << LoggerHelper::get_thread_id();
        print_stream << "] ";
        print_stream << buffer;
        print_stream << line_end_;
        
        LogLine log_line;
        log_line.level = log_level;
        log_line.msg = std::forward<std::string>(print_stream.str());
        
        // 内部自带锁
        log_cache_.emplace_back(std::move(log_line));
    }
    
    void log(LOG_LEVEL log_level, std::string&& msg){
        
        if(!inited_){
            return;
        }
        
        if(log_level < config_.level){
            return;
        }
        
        LogLine log_line;
        log_line.level = log_level;
        log_line.msg = std::move(msg);
        
        // 内部自带锁
        log_cache_.emplace_back(std::move(log_line));
    }
    
private:
    
    
    // 写入
    void write(){
        
        int64_t lines = log_cache_.size();
        
        while(lines > 0){
            
            LogLine log;
            
            if(!log_cache_.pop_front(log)){
                break;
            }
            
            size_t written_size = 0;
            
            if(config_.log_cb){
                config_.log_cb(log.level, log.msg.c_str());
            }
            else {
                if(!check_and_open()){
                    break;
                }
                
                if(!fp_){
                    break;
                }
                
                written_size = fwrite(log.msg.c_str(), 1, log.msg.size(), fp_);
                
                if(written_size != log.msg.size()){
                    break;
                }
            }
            
            file_size_bytes_ += log.msg.size();
            
            --lines;
        }
        
        if(fp_){
            fflush(fp_);
        }
    }
    
    bool check_and_open(){
        
        // 创建新的文件
        if(fp_ == nullptr || file_size_bytes_ >= config_.max_size_bytes){
            
            close();
            
            file_size_bytes_ = 0;
            
            std::string name = config_.dir + "/" + config_.prefix_name;
            name += "_";
            name += LoggerHelper::generate_file_name();
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
    
private:
    struct LogLine {
        std::string msg;
        LOG_LEVEL level = LOG_LEVEL_DEBUG;
    };
    
    Config config_;
    
    std::atomic<bool> inited_;
    
    DataQueue<LogLine> log_cache_;
    
    TaskQueue task_queue_;
    
    FILE* fp_ = nullptr;
    
    std::string line_end_ = "\r\n";
    
    size_t file_size_bytes_ = 0;
    
    LAZY_DISALLOW_COPY_AND_ASSIGN(Logger);
};


class LoggerStream {
public:
    
    LoggerStream(LOG_LEVEL log_level, const std::string& file, int line) : log_level_(log_level){
        print_stream_
        << "[" << LoggerHelper::now_to_string() << "] "
        << "(" << LoggerHelper::etract_file_name(file.c_str()) << ":" << line << ") "
        << ": "
        << "[" << LoggerHelper::level_to_string(log_level) << "] "
        << "[" << LoggerHelper::get_thread_id() << "] ";
    }
    
    ~LoggerStream(){
        print_stream_ << "\r\n";
        lazy::Logger::instance().log(log_level_, print_stream_.str());
    }
    
    LoggerStream& operator<<(bool v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(int v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(unsigned int v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(int64_t v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(uint64_t v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(const std::string& v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(const char* v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(const double& v) {
        print_stream_ << v;
        return *this;
    }
    
    LoggerStream& operator<<(void* v) {
        print_stream_ << v;
        return *this;
    }
private:
    LAZY_DISALLOW_COPY_AND_ASSIGN(LoggerStream);
    
    std::ostringstream print_stream_;
    LOG_LEVEL log_level_;
};

// use like std:out
#define LogDebug    lazy::LoggerStream(lazy::LOG_LEVEL_DEBUG, __FILE__, __LINE__)
#define LogInfo     lazy::LoggerStream(lazy::LOG_LEVEL_INFO, __FILE__, __LINE__)
#define LogWarn     lazy::LoggerStream(lazy::LOG_LEVEL_WARN, __FILE__, __LINE__)
#define LogError    lazy::LoggerStream(lazy::LOG_LEVEL_ERROR, __FILE__, __LINE__)



// use like printf
#define log_debug(format, ...)  lazy::Logger::instance().log(lazy::LOG_LEVEL_DEBUG,  __FILE__, __LINE__,format, ##__VA_ARGS__)
#define log_info(format, ...)   lazy::Logger::instance().log(lazy::LOG_LEVEL_INFO,  __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_warn(format, ...)   lazy::Logger::instance().log(lazy::LOG_LEVEL_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_error(format, ...)  lazy::Logger::instance().log(lazy::LOG_LEVEL_ERROR,  __FILE__, __LINE__,format, ##__VA_ARGS__)

#define log_debug_periodic(interval_ms, format, ...)\
do {                                                \
    static int64_t __time_ms__ = 0;                 \
    int64_t __now_ms__ = lazy::TimeUtil::NowMs();   \
    if(__now_ms__ - __time_ms__ >= interval_ms){    \
        __time_ms__ = __now_ms__;                   \
        log_debug(format,##__VA_ARGS__);            \
    }                                               \
} while(0)


#define log_info_periodic(interval_ms, format, ...) \
do {                                                \
    static int64_t __time_ms__ = 0;                 \
    int64_t __now_ms__ = lazy::TimeUtil::NowMs();   \
    if(__now_ms__ - __time_ms__ >= interval_ms){    \
        __time_ms__ = __now_ms__;                   \
        log_info(format,##__VA_ARGS__);             \
    }                                               \
} while(0)


#define log_warn_periodic(interval_ms, format, ...) \
do {                                                \
    static int64_t __time_ms__ = 0;                 \
    int64_t __now_ms__ = lazy::TimeUtil::NowMs();   \
    if(__now_ms__ - __time_ms__ >= interval_ms){    \
        __time_ms__ = __now_ms__;                   \
        log_warn(format,##__VA_ARGS__);             \
    }                                               \
} while(0)

#define log_error_periodic(interval_ms, format, ...)\
do {                                                \
    static int64_t __time_ms__ = 0;                 \
    int64_t __now_ms__ = lazy::TimeUtil::NowMs();   \
    if(__now_ms__ - __time_ms__ >= interval_ms){    \
        __time_ms__ = __now_ms__;                   \
        log_error(format,##__VA_ARGS__);            \
    }                                               \
} while(0)



}
#endif /* __LAZY_LOG_H_2024__ */
