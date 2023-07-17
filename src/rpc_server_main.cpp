#define RPCSERVER

#include "log.h"
#include "task.h"

int close_log = 0;
using namespace wut::zgy::cppnetwork;

void foo_1(){
    LOG_INFO("foo_1远程调用成功")
}

int foo_2(int arg1) {
    LOG_INFO("foo_2远程调用成功")
    return arg1 * arg1;
}

int foo_3(int arg1, int arg2){
    LOG_INFO("foo_3远程调用成功")
    return arg1 + arg2;
}

int main(){
    MyTask* task = MyTask::get_instance();
    task->add_task([&task](){
        auto server_fd = task->get_serverfd();
        // load_func将服务端定义的函数转换成接收Seirlizer*, const char*, int 的函数并以键值对的形式存放在map中
        server_fd->load_func("foo_1", foo_1);
        server_fd->load_func("foo_2", foo_2);
        server_fd->load_func("foo_3", foo_3);
    });
    task->run();
    return 0;
}