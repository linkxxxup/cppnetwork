#pragma once

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <memory>

#include "epoll.h"

namespace wut::zgy::cppnetwork{
class Socket{
public:
    Socket() = default;
    explicit Socket(int socket_fd): _socket_fd(socket_fd){}
    Socket(const std::string& ip, int port);
    ~Socket(){
        close();
    };
    int bind();
    int listen(int backlog);
    int close();
    int connect();
    int accept();
    int receive(int connect_fd, char *buf, size_t size);
    int send(const char *buf, size_t size);
    int set_send_buf(int size);
    int set_recv_buf(int size);
    int set_non_block();

public:
    int _socket_fd;
private:
    std::string _ip;
    int _port;

    friend class Server;
    friend class Client;
};

}