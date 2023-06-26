#define RPCCLIENT
#include <iostream>

#include "task.h"


int close_log = 0;
using namespace wut::zgy::cppnetwork;

int main() {
    // 获取日志配置
//    Conf conf("../conf/log.conf");
//    assert(conf.get_err_code() == 0);
//    // 初始化日志
//    Log *log = Log::get_instance();
//    assert(log->init(conf) == 0);
//    // 获取网络参数配置
//    Conf conf_net("../conf/net_data.conf");
//    CHECK_RET(conf_net.get_err_code()==0, "net_data.conf error");
//    // 初始化网络参数
//    auto net_data = NetData::get_instance();
//    CHECK_RET(net_data->init(conf_net)==0, "net_data init error");
//    // 获得服务套接字并初始化
//    auto cli_fd = RpcClient::get_instance();
//    CHECK_RET(cli_fd->init(net_data)==0, "client init error");
//    cli_fd->run();
//    std::string msg1 = cli_fd->call<void>("foo_1").error_msg();
//    std::string msg2 = cli_fd->call<int>("foo_2", 10).error_msg();
//    std::string msg3 = cli_fd->call<int>("foo_2", 10).error_msg();
//    cli_fd->close();
    MyTask* task = MyTask::get_instance();
    task->run();
    auto ret1 = task->call<void>("foo_1");
    auto ret2 = task->call<int>("foo_2", 10);
    auto ret3 = task->call<int>("foo_2", 20);
    task->close();
    std::cout << "调用foo_1的结果: " << ret1.val()  << " 调用信息: " << ret1.error_msg() << std::endl;
    std::cout << "调用foo_2(10)的结果: " << ret2.val() << " 调用信息: " << ret2.error_msg() << std::endl;
    std::cout << "调用foo_2(20)的结果: " << ret3.val() << " 调用信息: " << ret3.error_msg() << std::endl;

    return 0;
}