#pragma once
#include "net_data.h"
#include "socket.h"
namespace wut::zgy::cppnetwork{

class Client {
public:
    int init(NetData *net_data) ;
    int run();
    int close();
//    int parse() override; // 解析
//    int result() override; // 生成结果

protected:
    Client() = default;
    ~Client(){
        close();
    }
    Socket _cli_fd;
    NetData *_net_data;
private:
    std::string _ip;
    int _port;
};
}