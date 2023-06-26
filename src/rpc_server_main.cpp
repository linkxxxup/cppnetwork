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

int main(){
    MyTask* task = MyTask::get_instance();
    task->add_task([&task](){
        auto server_fd = task->get_serverfd();
        server_fd->load_func("foo_1", std::function<void()>(foo_1));
        server_fd->load_func("foo_2", std::function<int(int)> (foo_2));
    });
    task->run();
    return 0;
}