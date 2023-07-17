#define RPCCLIENT
#include <iostream>

#include "task.h"


int close_log = 0;
using namespace wut::zgy::cppnetwork;

int main() {
    MyTask* task = MyTask::get_instance();
    task->run();
    auto ret1 = task->call<void>("foo_1");
    auto ret2 = task->call<int>("foo_2", 10);
    auto ret3 = task->call<int>("foo_3", 20, 30);
    task->close();
    std::cout << "调用foo_1的结果: " << ret1.val()  << " 调用信息: " << ret1.error_msg() << std::endl;
    std::cout << "调用foo_2(10)的结果: " << ret2.val() << " 调用信息: " << ret2.error_msg() << std::endl;
    std::cout << "调用foo_3(20, 30)的结果: " << ret3.val() << " 调用信息: " << ret3.error_msg() << std::endl;

    return 0;
}