#define REGISTERSERVER

#include "registry.h"

int close_log = 0;
using namespace wut::zgy::cppnetwork;

int main(){
    Conf conf_log;
    Log *log;
    Conf conf_net;
    NetData* net_data;
    Registry* registry_fd;
    // 获取日志配置
    conf_log = Conf("../conf/log.conf");
    assert(conf_log.get_err_code() == 0);  //A
    // 初始化日志
    log = Log::get_instance();
    assert(log->init(conf_log) == 0);  //A->B
    // 获取网络参数配置
    conf_net = Conf("../conf/net_data.conf");   //C
    CHECK_RET(conf_net.get_err_code()==0, "net_data.conf error")
    // 初始化网络参数
    net_data = NetData::get_instance();
    CHECK_RET(net_data->init(conf_net)==0, "net_data init error");
    // 获得服务套接字并初始化
    registry_fd = Registry::get_instance();  //B,D->E
    CHECK_RET(registry_fd->init(net_data)==0, "Registry init error")
    registry_fd->run();

}
