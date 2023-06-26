#pragma once
#include <iostream>

#include <taskflow.hpp>

#include "conf.h"
#include "log.h"
#include "net_data.h"
#include "rpc_server.h"
#include "rpc_client.h"
#include "http_server.h"
//#include "http_client.h"

namespace wut::zgy::cppnetwork{

class MyTask{
public:
    static MyTask* get_instance(){
        static MyTask instance;
        return &instance;
    }

#ifdef RPCSERVER
    void add_task(std::function<void()> task){
        // 给rpc服务端添加函数
        auto G = _taskflow.emplace(task);
        _task_map.insert({'G', G});
    }
    RpcServer* get_serverfd(){
        return _rserver_fd;
    }
#endif
#ifdef RPCCLIENT
    template<typename R, typename... Params>
    value_t<R> call(const std::string &name, Params... ps){
        auto ret = _rclient_fd->call<R>(name, ps...);
        return ret;
    }
#endif
    
    void run(){
#ifdef RPCSERVER
        _task_map['A'].precede(_task_map['B']);
        _task_map['C'].precede(_task_map['D']);
        _task_map['E'].succeed(_task_map['B'],_task_map['D']);
        _task_map['G'].succeed(_task_map['E']);
        _task_map['G'].precede(_task_map['F']);
#endif
#ifdef RPCCLIENT
        _task_map['A'].precede(_task_map['B']);
        _task_map['C'].precede(_task_map['D']);
        _task_map['E'].succeed(_task_map['B'],_task_map['D']);
        _task_map['E'].precede(_task_map['F']);
#endif
#ifdef HTTPSERVER
        _task_map['A'].precede(_task_map['B']);
        _task_map['C'].precede(_task_map['D']);
        _task_map['E'].succeed(_task_map['B'],_task_map['D']);
        _task_map['E'].precede(_task_map['F']);
#endif
        _executor.run(_taskflow).wait();
    }

    void close(){
#ifdef RPCSERVER
        _rserver_fd->stop();
#endif
#ifdef RPCCLIENT
        _rclient_fd->close();
#endif
#ifdef HTTPSERVER
        _hserver_fd->stop();
#endif
#ifdef HTTPCLIENT
        _hclient_fd->close();
#endif
    }

private:
    MyTask() {
        auto A = _taskflow.emplace([this](){
            // 获取日志配置
            _conf_log = Conf("../conf/log.conf");
            assert(_conf_log.get_err_code() == 0);  //A
        });
        auto B = _taskflow.emplace([this](){
            // 初始化日志
            _log = Log::get_instance();
            assert(_log->init(_conf_log) == 0);  //A->B
        });

        auto C = _taskflow.emplace([this](){
            // 获取网络参数配置
            _conf_net = Conf("../conf/net_data.conf");   //C
            CHECK_RET(_conf_net.get_err_code()==0, "net_data.conf error");
        });
        auto D = _taskflow.emplace([this](){
            // 初始化网络参数
            _net_data = NetData::get_instance();
            CHECK_RET(_net_data->init(_conf_net)==0, "net_data init error");
        });

        auto E = _taskflow.emplace([this](){
            // 获得服务套接字并初始化
#ifdef RPCSERVER
            _rserver_fd = RpcServer::get_instance();  //B,D->E
            CHECK_RET(_rserver_fd->init(_net_data)==0, "rpc_server_fd init error");
#endif
#ifdef RPCCLIENT
            _rclient_fd = RpcClient::get_instance();  //B,D->E
            CHECK_RET(_rclient_fd->init(_net_data)==0, "rpc_client_fd init error");
#endif
#ifdef HTTPSERVER
            _hserver_fd = HttpServer::get_instance();  //B,D->E
            CHECK_RET(_hserver_fd->init(_net_data)==0, "http_server_fd init error");
#endif
#ifdef HTTPCLIENT
            _hclient_fd = HttpServer::get_instance();  //B,D->E
            CHECK_RET(_hclient_fd->init(_net_data)==0, "http_client_fd init error");
#endif
        });
        auto F = _taskflow.emplace([this](){
#ifdef RPCSERVER
            // rpc服务器开启
            _rserver_fd->run(); // E->F
#endif
#ifdef RPCCLIENT
            // rpc服务器开启
            _rclient_fd->run(); // E->F
#endif
#ifdef HTTPSERVER
        _hserver_fd->run();
#endif
#ifdef HTTPCLIENT
        _hclient_fd->run();
#endif
        });
        _task_map.insert({'A', A});
        _task_map.insert({'B', B});
        _task_map.insert({'C', C});
        _task_map.insert({'D', D});
        _task_map.insert({'E', E});
        _task_map.insert({'F', F});
        _task_number = 'F' + 1;
    }
    ~MyTask() = default;

    tf::Executor _executor;
    tf::Taskflow _taskflow;
    std::map<char, tf::Task> _task_map;
    char _task_number;

    Conf _conf_log;
    Log *_log;
    Conf _conf_net;
    NetData* _net_data;
    RpcServer* _rserver_fd;
    RpcClient* _rclient_fd;
    HttpServer* _hserver_fd;
//    HttpClient* _hclient_fd; // http客户端还没开发，直接用postman发请求了
};
}