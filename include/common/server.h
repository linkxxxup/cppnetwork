#pragma once

#include <memory>

#include "net_data.h"
#include "socket.h"
#include "epoll.h"
#include "thread_pool.h"

namespace wut::zgy::cppnetwork{

class Server{
public:
    int init(NetData *net_data){
        _net_data = net_data;
        _ip = _net_data->_ip;
        _port = _net_data->_port;
        _listen_fd = Socket(_ip, _port);
        _is_et_conn = _net_data->_is_et_conn;
        _is_et_lis = _net_data->_is_et_lis;
        _thread_pool = std::make_unique<ThreadPool>(_net_data->_thread_num_max);
        _epoll_fd = std::make_unique<Epoller>(net_data->_max_event);
        return 0;
    }

    int run(); // run中调用epoll_wait()
    int epoll_wait();
    int stop();
    int accept();
    virtual int receive(int sockfd) = 0;
    virtual int deal_read(int sockfd, int len) = 0; // 处理读事件
    virtual int deal_write(int sockfd) = 0; // 处理写事件

protected:
    Server() = default;
    virtual ~Server(){
        _listen_fd.close();
    }
    NetData* _net_data;
    Socket _listen_fd;
    Socket _connect_fd;
    int _close;
    bool _is_et_conn;
    bool _is_et_lis;
    std::unique_ptr<ThreadPool> _thread_pool;
    std::unique_ptr<Epoller> _epoll_fd;
    std::string _ip;
    int _port;
};
}