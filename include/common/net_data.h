#pragma once

#include <iostream>
#include <sstream>
#include <functional>
#include <epoll.h>

#include "conf.h"
#include "log.h"


extern int close_log;
namespace wut::zgy::cppnetwork{

class NetData{
public:
    enum NET_FLAG{
        SERVER,
        CLIENT
    };

    enum NET_MODE{
        RPCMODE,
        HTTPMODE
    };

    // rpc
    enum RPC_ERR_CODE{
        RPC_ERR_SUCCESS = 0,
        RPC_ERR_FUNC_NOT_BIND,
        RPC_ERR_FAILED
    };

    // http
    enum HTTP_ERR_CODE{
        NO_REQUEST,  // 只有请求体的一部分
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum METHOD{
        GET,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum LINE_STATUS{
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    static NetData *get_instance(){
        static NetData instance;
        return &instance;
    }

    int init(Conf &conf){
#ifdef HTTPSERVER
        _net_flag = SERVER;
#endif
#ifdef HTTPCLIENT
        _net_flag = CLIENT;
#endif
#ifdef RPCSERVER
        _net_flag = SERVER;
#endif
#ifdef RPCCLIENT
        _net_flag = CLIENT;
#endif
        if(_net_flag == SERVER){
            _ip = conf.get("server.ip");
            _port = std::stoi(conf.get("server.port"));
            // 监听socket不能注册oneshot，否则程序只能处理一个客户连接，以后后续客户端的连接请求将不再触发listenfd上的EPOLL事件
            _is_run = false;
            _use_thread_pool = false;
            _thread_num_max = -1;
            _max_event = std::stoi(conf.get("server.max_event"));
            _is_et_lis = false;
            _is_et_conn = false;
            if(std::stoi(conf.get("server.event_listen_mode")) == 1){
                _is_et_lis = true;
            }
            if(std::stoi(conf.get("server.event_connect_mode")) == 1){
                _is_et_conn = true;
            }
            _thread_num_max = std::stoi(conf.get("server.thread_num_max"));
            if(_thread_num_max > 0){
                _use_thread_pool = true;
            }
        }
        if(_net_flag == CLIENT){
            _ip = conf.get("client.ip");
            _port = std::stoi(conf.get("client.port"));
        }
        return 0;
    }

private:
    NetData() = default;
    ~NetData() {
        _is_run = false;
    }

    int _net_flag;
    int _net_mode;
    std::string _ip;
    int _port;
    std::string _target_ip;
    int _target_port;
    bool _is_run;
    bool _use_thread_pool;
    int _thread_num_max;
    int _max_event;
    bool _is_et_conn;
    bool _is_et_lis;


    friend class Client;
    friend class Server;
};
}
