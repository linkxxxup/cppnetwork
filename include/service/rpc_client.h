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
        value_t<R> call(const std::string &name, Params... ps){
            using args_type = std::tuple<typename std::decay<Params>::type...>;
            args_type args = std::make_tuple(ps...);
            Serializer ds;
            ds << name;
            package_params(ds, args);
            return net_call<R>(ds);
        }
        template<typename R>
        value_t<R> call(const std::string& name){
            Serializer ds;
            ds << name;
            return net_call<R>(ds);
        }

        template<typename R>
        value_t<R> net_call(Serializer &ds){
            _cli_fd.send(ds.data(), ds.size());
            char ret[100];
            memset(ret, 0, sizeof(ret));
            int len = _cli_fd.receive(_cli_fd._socket_fd, ret, sizeof (ret));
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

    private:
        RpcClient() = default;
        ~RpcClient() = default;

    };
}