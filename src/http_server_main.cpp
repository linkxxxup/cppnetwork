#define HTTPSERVER

#include "task.h"

int close_log = 0;
using namespace wut::zgy::cppnetwork;

int main(){
    MyTask* task = MyTask::get_instance();
    task->run();

    return 0;
}