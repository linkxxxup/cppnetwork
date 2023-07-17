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
        std::string _port;
        std::vector<std::string> _server_list;
        std::map<std::string, int> _resource_count;

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
    bool register_(int sockfd, Serializer*, int len);
    void deregister();
    void get_server();
    void discover_server();
    void update_server();

    Registry() = default;
    ~Registry() override = default;
    // map存储sockfd和对应的server信息
    std::map<int, ServerItem> _server_map;
    std::mutex _mutex_map;
    std::chrono::duration<int, std::milli> _timer;
    // map存储sockfd和对应任务
    std::map<int, EveryTask> _task_map;
    std::mutex _mutex_task;

};
}