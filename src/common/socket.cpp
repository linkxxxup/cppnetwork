#include "socket.h"
#include "log.h"

extern int close_log;
namespace wut::zgy::cppnetwork{
    Socket::Socket(const std::string &ip, int port) : _ip(ip), _port(port), _socket_fd(0){}
    int Socket::bind() {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof (sockaddr));
        sockaddr.sin_family = AF_INET;
        if(_ip == "localhost" || _ip == "127.0.0.1" || _ip == ""){
            sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        }else{
            sockaddr.sin_addr.s_addr = inet_addr(_ip.c_str());
        }
        sockaddr.sin_port = htons(_port);
        if(::bind(_socket_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1){
            LOG_ERROR("bind error is %s", strerror(errno));
            return 1;
        }else{
            return 0;
        }
    }

    int Socket::listen(int backlog) {
        if(::listen(_socket_fd, backlog) == -1){
            LOG_ERROR("listen error is %s", strerror(errno));
            return 1;
        }else{
            return 0;
        }
    }

    int Socket::connect() {
        struct sockaddr_in sockaddr;
        memset(&sockaddr, 0, sizeof (sockaddr));
        sockaddr.sin_family = AF_INET;
        if(_ip == "localhost" || _ip == "127.0.0.1" || _ip == ""){
            sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        }else{
            sockaddr.sin_addr.s_addr = inet_addr(_ip.c_str());
        }
        sockaddr.sin_port = htons(_port);
        if(::connect(_socket_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1){
            LOG_ERROR("connect error is %s", strerror(errno));
            return 1;
        }else{
            return 0;
        }
    }

    int Socket::close(){
        if(_socket_fd > 0){
            ::close(_socket_fd);
            _socket_fd = 0;
        }
        return 0;
    }

    int Socket::accept() {
        int connect_fd = ::accept(_socket_fd, nullptr, nullptr);
        if(connect_fd == -1 ){
            LOG_ERROR("accept error is %s", strerror(errno));
            return -1;
        }else{
            return connect_fd;
        }
    }

    int Socket::receive(int connect_fd, char * buf, size_t size){
        // et mode
        int len = ::recv(connect_fd, buf, size, 0);
        return len;
    }

    int Socket::send(const char *buf, size_t size) {
        if(::send(_socket_fd, buf, size, 0) < 0){
            LOG_ERROR("send error is %s", strerror(errno));
            return 1;
        }else{
            return 0;
        }
    }

    int Socket::set_send_buf(int size) {
        int size_buf = size;
        if(::setsockopt(_socket_fd, SOL_SOCKET, SO_SNDBUF, &size_buf, sizeof(size_buf)) < 0){
            LOG_ERROR("set_send_buf error is %s", strerror(errno));
            return 1;
        }else{
            return 0;
        }
    }

    int Socket::set_recv_buf(int size) {
        int size_buf = size;
        if(::setsockopt(_socket_fd, SOL_SOCKET, SO_RCVBUF, &size_buf, sizeof(size_buf)) < 0){
            LOG_ERROR("set_send_buf error is %s", strerror(errno));
            return 1;
        }else{
            return 0;
        }
    }

    int Socket::set_non_block() {
        int flag = ::fcntl(_socket_fd, F_GETFL, 0);
        if(flag < 0){
            LOG_ERROR("set_non_block error is %s", strerror(errno));
            return 1;
        }
        flag |= O_NONBLOCK;
        if(fcntl(_socket_fd, F_SETFL, flag) < 0){
            LOG_ERROR("set_non_block error is %s", strerror(errno));
            return 1;
        }
        return 0;
    }

}