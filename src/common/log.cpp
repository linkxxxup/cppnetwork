#include "log.h"

namespace wut::zgy::cppnetwork{

Log::Log()
{
    _count = 0;
    _is_async = false;
}

Log::~Log()
{
    if (_fp != nullptr)
    {
        fclose(_fp);
    }
}


//异步需要设置阻塞队列的长度，同步不需要设置
int Log::init(Conf &conf)
{
    int mode = std::stoi(conf.get("log.mode"));
    std::string file_name = conf.get("log.dir");
    const char* cfile_name = file_name.c_str();
    int max_queue_size = std::stoi(conf.get("log.max_queue_size"));
    int log_buf_size = std::stoi(conf.get("log.log_buf_size"));
    int split_lines = std::stoi(conf.get("log.split_lines"));
    //如果mode==1,则设置为异步
    if (mode == 1)
    {
        _log_queue = new BlockDeque<std::string>(max_queue_size);
        _is_async = true;
        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        std::thread log_thread(flush_log_thread);
        log_thread.detach();
    }

    _log_buf_size = log_buf_size;
    _buf = new char[_log_buf_size];
    memset(_buf, '\0', _log_buf_size);
    _split_lines = split_lines;

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;


    const char *p = strrchr(cfile_name, '/');
    char log_full_name[300] = {0};

    if (p == nullptr)
    {
        snprintf(log_full_name, 300, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, cfile_name);
    }
    else
    {
        strcpy(_log_name, p + 1);
        strncpy(_dir_name, cfile_name, p - cfile_name + 1);
        snprintf(log_full_name, 300, "%s%d_%02d_%02d_%s", _dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, _log_name);
    }

    _today = my_tm.tm_mday;

    _fp = fopen(log_full_name, "a");
    if (_fp == nullptr)
    {
        return 1;
    }

    return 0;
}

void Log::write_log(int level, const char *file, size_t line, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
        case 0:
            strcpy(s, "[DEBUG]:");
            break;
        case 1:
            strcpy(s, "[INFO]:");
            break;
        case 2:
            strcpy(s, "[WARN]:");
            break;
        case 3:
            strcpy(s, "[ERROR]:");
            break;
        default:
            strcpy(s, "[INFO]:");
            break;
    }
    //写入一个log，对_count++, _split_lines最大行数
    _mutex.lock();
    _count++;

    if (_today != my_tm.tm_mday || _count % _split_lines == 0) //everyday log
    {

        char new_log[50] = {0};
        fflush(_fp);
        fclose(_fp);
        char tail[16] = {0};

        // 生成日志名称
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (_today != my_tm.tm_mday)
        {
            snprintf(new_log, 50, "%s%s%s", _dir_name, tail, _log_name);
            _today = my_tm.tm_mday;
            _count = 0;
        }
        else
        {
            snprintf(new_log, 50, "%s%s%s.%lld", _dir_name, tail, _log_name, _count / _split_lines);
        }
        _fp = fopen(new_log, "a");
    }

    _mutex.unlock();
    va_list valst;
    va_start(valst, format);
    std::string log_str;
    _mutex.lock();

    //获取工作路径
    char work_path[200];
    getcwd(work_path, sizeof (work_path));
    auto len = strlen(work_path);
    //写入的具体时间内容格式
    int n = snprintf(_buf, _log_buf_size, "%d-%02d-%02d %02d:%02d:%02d.%06ld .%s:%zu %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, file + len - 4, line, s); //减去src的长度+1
    int m = vsnprintf(_buf + n, _log_buf_size - n - 1, format, valst);
    _buf[n + m] = '\n';
    _buf[n + m + 1] = '\0';
    log_str = _buf;

    _mutex.unlock();

    if (_is_async && !_log_queue->full())
    {
        // 生产者向任务队列中添加任务
        _log_queue->push_back(log_str);
    }
    else
    {
        _mutex.lock();
        fputs(log_str.c_str(), _fp);
        _mutex.unlock();
    }

    va_end(valst);
}


void* Log::async_write_log()
{
    std::string single_log;
    //从阻塞队列中取出一个日志string，写入文件
    while (_log_queue->pop(single_log))
    {
        _mutex.lock();
        fputs(single_log.c_str(), _fp);
        _mutex.unlock();
    }
    return nullptr;
}

void Log::flush()
{
    _mutex.lock();
    //强制刷新写入流缓冲区
    fflush(_fp);
    _mutex.unlock();
}

}