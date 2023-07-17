#include "registry.h"

namespace wut::zgy::cppnetwork{
int Registry::receive(int sockfd) {
    _mutex_task.lock();
    _task_map.insert({sockfd, EveryTask(sockfd)});
    _mutex_task.unlock();
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

int Registry::deal_read(int sockfd, int len) {
    EveryTask &task = _task_map.at(sockfd);
    //将recv_buf中的数据转换成ServerItem
    StreamBuffer io_dev(task._recv_buf, len);
    //重置接收缓冲区
    memset(task._recv_buf, 0, sizeof(task._recv_buf));
    Serializer ds(io_dev);
    if(ds.size() == 0){
        LOG_INFO("ds has no data")
        return -1;
    }
    if(register_(sockfd, &ds, len)){
        LOG_INFO("register success: sockfd[%d]", sockfd);
    }else{
        LOG_INFO("no data receive")
        return -1;
    }
}

int Registry::deal_write(int sockfd) {

}

bool Registry::register_(int sockfd, Serializer* ds, int len) {
    ServerItem st;
    st._sockfd = sockfd;
    *ds >> st._type;
    *ds >> st._ip;
    *ds >> st._port;
    while(!ds->_iodevice.is_eof()){
        std::string temp;
        *ds >> temp;
        st._server_list.push_back(temp);
    }
    if(st._server_list.size() > 0) {
        _mutex_map.lock();
        _server_map.insert({sockfd, st});
        _mutex_map.unlock();
        return true;
    }
    else return false;
}
}