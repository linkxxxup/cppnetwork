#define HTTPSERVER

#include "task.h"

int close_log = 0;
using namespace wut::zgy::cppnetwork;

int main(){
//    // 获取日志配置
//    Conf conf("../conf/log.conf");
//    assert(conf.get_err_code() == 0);  //A
//    // 初始化日志
//    Log *log = Log::get_instance();
//    assert(log->init(conf) == 0);  //A->B
//    // 获取网络参数配置
//    Conf conf_net("../conf/net_data.conf");   //C
//    CHECK_RET(conf_net.get_err_code()==0, "net_data.conf error");
//    // 初始化网络参数
//    auto net_data = NetData::get_instance(); //C->D
//    CHECK_RET(net_data->init(conf_net)==0, "net_data init error");
//    // 获得服务套接字并初始化
//    auto server_fd = HttpServer::get_instance();  //B,D->E
//    CHECK_RET(server_fd->init(net_data)==0, "server_fd init error");
//    server_fd->run(); // E->F
    MyTask* task = MyTask::get_instance();
    task->run();

    return 0;
}