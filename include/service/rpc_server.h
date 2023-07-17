#pragma once

#include <map>

#include "server.h"
#include "serializer.h"
#include "rpc_utils.h"

namespace wut::zgy::cppnetwork{
class RpcServer : public Server{
public:
    static RpcServer *get_instance()
    {
        static RpcServer instance;
        return &instance;
    }

//        int send() override;
    int receive(int sockfd) override;
    int deal_read(int sockfd, int len) override;
    int deal_write(int sockfd) override;
    void connect_registry();

    class EveryTask{
    public:
        Serializer* _ret;
        char _recv_buf[100];
        char _send_buf[100];
        int _socket;
        EveryTask(int socket): _socket(socket){
            memset(_recv_buf, 0, sizeof(_recv_buf));
            memset(_send_buf, 0, sizeof(_send_buf));
        }
        ~EveryTask() = default;
    };

    // 服务端调用用来绑定函数的方法, 将服务端的函数加载到_func_map中
    template<typename F>
    void load_func(std::string name, F func);
    template<typename F, typename S>
    void load_func(std::string name, F func, S* s);

    Serializer* deal(const std::string& name, const char* data, int len);

    template<typename F>
    void callproxy(F fun, Serializer* pr, const char* data, int len);
    template<typename F, typename S>
    void callproxy(F fun, S* s, Serializer* pr, const char* data, int len);

    // 函数指针
    template<typename R, typename... Params>
    void callproxy_(R(*func)(Params...), Serializer* pr, const char* data, int len);
    // 类成员函数指针
    template<typename R, typename C, typename S, typename... Params>
    void callproxy_(R(C::* func)(Params...), S* s, Serializer* pr, const char* data, int len);
    // functional
    template<typename R, typename... Params>
    void callproxy_(std::function<R(Params... ps)> func, Serializer* pr, const char* data, int len);

private:
    RpcServer() = default;
    ~RpcServer() override = default;
    bool register_(); // 向注册中心发送服务器端信息
    Socket _register_fd;
    // 存储服务端所有函数的哈希表
    std::unordered_map<std::string, std::function<void(Serializer*, const char *, int)>> _func_map;
    std::map<int, EveryTask> _task_map;
    std::mutex _mutex;
};

template<typename F>
void RpcServer::load_func(std::string name, F func) {
    _func_map[name] = std::bind(&RpcServer::callproxy<F>, this, func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}
template<typename F, typename S>
void RpcServer::load_func(std::string name, F func, S *s) {
    _func_map[name] = std::bind(&RpcServer::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}


template<typename F>
void RpcServer::callproxy(F fun, wut::zgy::cppnetwork::Serializer *pr, const char *data, int len) {
    callproxy_(fun, pr, data, len);
}
template<typename F, typename S>
void RpcServer::callproxy(F fun, S *s, wut::zgy::cppnetwork::Serializer *pr, const char *data, int len) {
    callproxy_(fun, s, pr, data, len);
}

template<typename R, typename ...Params>
void RpcServer::callproxy_(R (*func)(Params...), wut::zgy::cppnetwork::Serializer *pr, const char *data, int len) {
    callproxy_(std::function<R(Params...)>(func), pr, data, len);
}

template<typename R, typename C, typename S, typename ...Params>
void RpcServer::callproxy_(R (C::*func)(Params...), S *s, wut::zgy::cppnetwork::Serializer *pr, const char *data,
                           int len) {
    using args_type = std::tuple<typename std::decay<Params>::type...>;

    Serializer ds(StreamBuffer(data, len));
    constexpr auto N = std::tuple_size<typename std::decay<args_type>::type>::value;
    args_type args = ds.get_tuple < args_type >(std::make_index_sequence<N>{});

    auto ff = [=](Params... ps)->R {
        return (s->*func)(ps...);
    };
    typename type_xx<R>::type r = call_helper<R>(ff, args);

    value_t<R> val;
    val.set_code(NetData::RPC_ERR_CODE::RPC_ERR_SUCCESS);
    val.set_val(r);
    (*pr) << val;
}

template<typename R, typename ...Params>
void RpcServer::callproxy_(std::function<R(Params...)> func, wut::zgy::cppnetwork::Serializer *pr, const char *data,
                           int len) {
    // args_type为对应Params的元组类型
    using args_type = std::tuple<typename std::decay<Params>::type...>;

    Serializer ds(StreamBuffer(data, len));
    constexpr auto N = std::tuple_size<typename std::decay<args_type>::type>::value;
    // 将字符串data转换成相应的形参类型然后打包成一个元组
    args_type args = ds.get_tuple<args_type>(std::make_index_sequence<N>{});

    typename type_xx<R>::type r = call_helper<R>(func, args);

    value_t<R> val;
    val.set_code(NetData::RPC_ERR_CODE::RPC_ERR_SUCCESS);
    val.set_val(r);
    val.set_msg("request success");
    (*pr) << val;
}
}
