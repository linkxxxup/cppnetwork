#pragma once

#include <map>
#include <chrono>
#include <mutex>

#include "server.h"
#include "serializer.h"

namespace wut::zgy::cppnetwork{
class Registry : public Server{
public:
    enum SERVERTYPE{
        HTTP,
        RPC
    };
    enum RECVTYPE{
        REG,
        DEREG,
        FINDSERVER
    };
    static Registry* get_instance(){
        static Registry instance;
        return &instance;
    }

    int receive(int sockfd) override;
    int deal_read(int sockfd, int len) override;
    int deal_write(int sockfd) override;

    struct ServerItem{
        //存储每个服务的sockfd
        //服务类型（http/rpc）
        //所在的地址
        //包括的服务（rpc包括服务函数名，http为请求的资源页）
        //各种服务调用的次数
        int _sockfd;
        SERVERTYPE _type;
        std::string _ip;
        int _port;
        std::vector<std::string> _server_list;
    };

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


private:
    // 注册服务
    bool register_(int sockfd, Serializer*, int len);
    // 删除服务
    void deregister();
    // 发现服务，返回服务器地址给客户端
    int discover_server(Serializer*, int len);
    void update_server();

    Registry() = default;
    ~Registry() override = default;
    // map存储sockfd和对应的server信息
    std::map<int, ServerItem> _server_map;
    std::mutex _mutex_server;
    std::chrono::duration<int, std::milli> _timer;
    // map存储sockfd和对应任务
    std::map<int, EveryTask> _task_map;
    std::mutex _mutex_task;
    // map存储服务名和sockfd
    std::map<std::string, std::vector<int>> _addr_map;
    std::mutex _mutex_addr;
    // map存储服务名和调用次数
    std::map<std::string, int> _count_map;
    std::mutex _mutex_count;
};
}