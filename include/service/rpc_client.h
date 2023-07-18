#pragma once

#include "client.h"
#include "rpc_utils.h"
#include "serializer.h"

namespace wut::zgy::cppnetwork{
class RpcClient : public Client{
public:
    static RpcClient* get_instance(){
        static RpcClient rpc_cli;
        return &rpc_cli;
    }
    template<typename R, typename... Params>
    value_t<R> call(const std::string &name, Params... ps);
    template<typename R>
    value_t<R> call(const std::string& name);
    template<typename R>
    value_t<R> net_call(Serializer &ds);

private:
    RpcClient() = default;
    ~RpcClient() = default;
    void get_server(const std::string& name);

    std::string _server_ip;
    int _server_port;
    Socket _server_fd;
};

template<typename R, typename... Params>
value_t<R> RpcClient::call(const std::string &name, Params... ps){
    get_server(name);
    using args_type = std::tuple<typename std::decay<Params>::type...>;
    args_type args = std::make_tuple(ps...);
    Serializer ds;
    ds << name;
    package_params(ds, args);
    return net_call<R>(ds);
}

template<typename R>
value_t<R> RpcClient::call(const std::string& name){
    get_server(name);
    Serializer ds;
    ds << name;
    return net_call<R>(ds);
}

template<typename R>
value_t<R> RpcClient::net_call(Serializer &ds){
    _server_fd.send(ds.data(), ds.size());
    char ret[100];
    memset(ret, 0, sizeof(ret));
    int len = _server_fd.receive(_server_fd._socket_fd, ret, sizeof (ret));
    value_t<R> val;
    if(len == 0){
        val.set_code(NetData::RPC_ERR_CODE::RPC_ERR_FAILED);
        val.set_msg("server return no message");
        LOG_ERROR("client recv no message");
        return val;
    }
    ds.clear();
    ds.write_raw_data(ret, len);
    ds.reset();
    ds >> val;
    return val;
}

void RpcClient::get_server(const std::string& name) {
    Serializer ds;
    ds << 2; // 注册中心发现函数的标志位
    ds << name;
    assert(_cli_fd.send(ds.data(), ds.size()) == 0);
    char ret[100];
    memset(ret, 0, sizeof(ret));
    int len = _cli_fd.receive(_cli_fd._socket_fd, ret, sizeof(ret));
    ds.clear();
    ds.write_raw_data(ret, len);
    ds.reset();
    int flag;
    ds >> flag;
    if(flag == 0){
        _server_ip.clear();
        _server_port = 0;
        ds >> _server_ip;
        ds >> _server_port;
        _server_fd = Socket(_server_ip, _server_port);
        if((_server_fd._socket_fd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
            LOG_ERROR("%s", strerror(errno));
            abort();
        }
        _server_fd.set_recv_buf(10 * 1024);
        _server_fd.set_send_buf(10 * 1024);
        _server_fd.connect();
    }else{
        LOG_ERROR("no have func: %s", name.c_str());
        abort();
    }
}
}