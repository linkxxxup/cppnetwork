#include "rpc_server.h"

namespace wut::zgy::cppnetwork{
int RpcServer::receive(int sockfd){
    _mutex.lock();
    _task_map.insert({sockfd, EveryTask(sockfd)});
    _mutex.unlock();
    EveryTask &task = _task_map.at(sockfd);
    char *buf = task._recv_buf;
    memset(task._recv_buf, 0, sizeof(task._recv_buf));
    int len;
    int pos = 0;
    while(true){
        len = _listen_fd.receive(sockfd, buf+pos, sizeof(task._recv_buf)-pos);
        if(len < 0){
            if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
                LOG_INFO("EAGAIN is trigger")
                return pos;
            }
            return pos;
        }else if(len == 0){
            return pos;
        }else{
            pos += len;
        }
    }
}

int RpcServer::deal_read(int socket, int len) {
    EveryTask &task = _task_map.at(socket);
    StreamBuffer io_dev(task._recv_buf, len);
    //重置接收缓冲区
    memset(task._recv_buf, 0, sizeof(task._recv_buf));
    Serializer ds(io_dev);
    std::string func_name;
    ds >> func_name;
    task._ret = RpcServer::deal(func_name, ds.current(), ds.size() - func_name.size());
    if(task._ret->size() > 0){
        //重置oneshot
        if(_is_et_conn){
            _epoll_fd->mod_fd(socket, EPOLLOUT|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
        }else{
            _epoll_fd->mod_fd(socket, EPOLLOUT|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
        }
        return 0;
    }else{
        return -1;
    }
}

int RpcServer::deal_write(int sockfd) {
    EveryTask &task = _task_map.at(sockfd);
    memcpy(task._send_buf, task._ret->data(), task._ret->size());
    int len = ::send(sockfd, task._send_buf, task._ret->size(), 0);
    memset(task._send_buf, 0, sizeof(task._send_buf));
    if(len > 0){
        //重置oneshot
        if(_is_et_conn){
            _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
        }else{
            _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
        }
        return len;
    }else{
        return -1;
    }
}

Serializer *RpcServer::deal(const std::string& name, const char *data, int len) {
    Serializer* ds = new Serializer();
    if (_func_map.find(name) == _func_map.end()) {
        (*ds) << value_t<int>::code_type(NetData::RPC_ERR_CODE::RPC_ERR_FUNC_NOT_BIND);
        (*ds) << value_t<int>::msg_type("function not bind: " + name);
        LOG_ERROR("function not bind");
        return ds;
    }
    auto fun = _func_map[name];
    fun(ds, data, len);
    ds->reset();
    return ds;
}

void RpcServer::connect_registry() {
    // server作为客户端连接注册中心
    std::string register_ip = _net_data->_register_ip;
    int register_port = _net_data->_register_port;
    Socket register_fd = Socket(register_ip, register_port);
    if((register_fd._socket_fd = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
        LOG_ERROR("%s", strerror(errno))
        abort();
    }
    register_fd.set_send_buf(10 * 1024);
    register_fd.set_recv_buf(10 * 1024);
    int res = register_fd.connect();
    if(res == 1){
        LOG_ERROR("register can not connect")
        abort();
    }else{
        _register_fd = register_fd;
        LOG_INFO("register connected!")
    }
    // 向注册中心发送服务器端信息
    assert(register_());
}

bool RpcServer::register_(){
    Serializer ds;
    ds << _net_data->_net_flag;
    ds << _ip;
    ds << _port;
    for(auto & it : _func_map){
        ds << it.first;
    }
    int res = _register_fd.send(ds.data(), ds.size());
    if(res == 1){
        LOG_ERROR("register can not connect")
    }else{
        LOG_INFO("register data send!")
    }
    char recv_buf[20];
    memset(recv_buf, 0, sizeof(recv_buf));
    res = _register_fd.receive(_register_fd._socket_fd, recv_buf, sizeof(recv_buf));
    std::string res_info = recv_buf;
    if(res > 0 && res_info == "register:0"){
        LOG_INFO("register success!")
        return true;
    }else{
        LOG_INFO("register failed!")
        return false;
    }
}

}