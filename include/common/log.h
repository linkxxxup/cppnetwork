#pragma once

#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdarg>
#include <thread>
#include <mutex>
#include <functional>

#include "conf.h"
#include "block_queue.h"

namespace wut::zgy::cppnetwork{
// TODO: 调试时第一行日志不输出
// TODO: 第一次http请求时不生成日志，第二次请求后会生成第一次的日志 6.11
class Log
{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    static void* flush_log_thread(){
        Log::get_instance()->async_write_log();
        return nullptr;
    }

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    int init(Conf &conf);
    void write_log(int level, const char *file, size_t line, const char *format, ...);
    void flush();

private:
    Log();
    virtual ~Log();
    // 异步模式下，工作线程调用函数写入日志文件
    void *async_write_log();

private:
    char _dir_name[128]; //路径名
    char _log_name[128]; //log文件名
    int _split_lines;  //日志最大行数
    int _log_buf_size; //日志缓冲区大小
    long long _count;  //日志行数记录
    int _today;        //因为按天分类,记录当前时间是那一天
    FILE *_fp;         //打开log的文件指针
    char *_buf;
    BlockDeque<std::string> *_log_queue; //阻塞队列
    bool _is_async;                  //是否同步标志位
    std::mutex _mutex;
};
}

#define LOG_DEBUG(format, ...) if(0 == close_log) {wut::zgy::cppnetwork::Log::get_instance()->write_log(0, format, ##__VA_ARGS__); wut::zgy::cppnetwork::Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == close_log) {wut::zgy::cppnetwork::Log::get_instance()->write_log(1, __FILE__, __LINE__, format, ##__VA_ARGS__); wut::zgy::cppnetwork::Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == close_log) {wut::zgy::cppnetwork::Log::get_instance()->write_log(2, format, ##__VA_ARGS__); wut::zgy::cppnetwork::Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == close_log) {wut::zgy::cppnetwork::Log::get_instance()->write_log(3, __FILE__, __LINE__, format, ##__VA_ARGS__); wut::zgy::cppnetwork::Log::get_instance()->flush();}

#define CHECK_RET(ret, info) \
    if(!(ret)){ \
        LOG_ERROR("CONF INFO: %s", info); \
        abort(); \
    }
