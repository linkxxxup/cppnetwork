#include "server.h"

namespace wut::zgy::cppnetwork{

int Server::run() {
    if((_listen_fd._socket_fd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        LOG_ERROR("%s", strerror(errno));
        return 1;
    }
    _listen_fd.set_send_buf(20 * 1024);
    _listen_fd.set_recv_buf(20 * 1024);
    _listen_fd.set_non_block();
    _listen_fd.bind();
    _listen_fd.listen(1024);

    int flag = EPOLLIN|EPOLLRDHUP|EPOLLERR;
    if(_is_et_lis){
        flag |= EPOLLET;
    }
    _epoll_fd->add_fd(_listen_fd._socket_fd, flag);
    CHECK_RET(epoll_wait()==0, "epoll wait error");
    return 0;
}

int Server::epoll_wait() {
    while(!_close){
        int event_num = _epoll_fd->wait();
        if(event_num < 0){
            LOG_ERROR("epoll wait error, err is %s", strerror(errno));
            _close = 1;
            return 1;
        }
        for(int i = 0; i < event_num; ++i){
            int sockfd = _epoll_fd->get_event_fd(i);
            if(sockfd == _listen_fd._socket_fd){
                // 为了调试，先不使用线程
                accept();
                // 用线程池中线程处理
//                    _thread_pool->add_task([this] { return this->accept(); });
            }else if(_epoll_fd->get_events(i) & EPOLLIN){
                // 为了调试，先不使用线程
                int len = receive(sockfd);
                if(len < 0){
                    _epoll_fd->del_fd(sockfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT);
                    ::close(sockfd);
                }
                if(len > 0){
                    int ret = deal_read(sockfd, len);
                }
                // 用线程池中线程处理
//                    _thread_pool->add_task([this, sockfd] {
//                        int len = receive(sockfd);
//                        if(len < 0){
//                            _epoll_fd->del_fd(sockfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT);
//                            ::close(sockfd);
//                        }
//                        if(len > 0){
//                            deal_read(sockfd, len);
//                        }
//                        if(len == 0){
//                            if(_is_et_conn){
//                                _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLET|EPOLLERR|EPOLLRDHUP);
//                            }else{
//                                _epoll_fd->mod_fd(sockfd, EPOLLIN|EPOLLONESHOT|EPOLLERR|EPOLLRDHUP);
//                            }
//                        }
//                    });
            }else if(_epoll_fd->get_events((i)) & EPOLLOUT){
//                    _thread_pool->add_task([this, sockfd]{
//                        int len = deal_write(sockfd);
//                        if(len < 0){
//                            LOG_INFO("server send failed");
//                        }
//                    });
                // 不使用线程
                int len = deal_write(sockfd);
                if(len <= 0){
                    LOG_INFO("server send failed");
                }
            }else if(_epoll_fd->get_events((i)) & EPOLLERR){
                _epoll_fd->del_fd(sockfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT);
                close(sockfd);
                sockfd = 0;
                LOG_ERROR("EPOLLERR is trigger");
            }else if(_epoll_fd->get_events((i)) & EPOLLRDHUP || _epoll_fd->get_events((i)) & EPOLLHUP){
                _epoll_fd->del_fd(sockfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLRDHUP | EPOLLHUP | EPOLLONESHOT);
                close(sockfd);
                sockfd = 0;
                LOG_INFO("EPOLLHUP is trigger");
            }else{
                //pass;
            }
        }
    }
    LOG_INFO("server is close");
    return 0;
}

int Server::accept(){
    _connect_fd._socket_fd = _listen_fd.accept();
    if(_connect_fd._socket_fd < 0) {
        LOG_ERROR("Server::accept()中返回_listen_fd.accept()错误")
        return 1;
    }
    int flag = EPOLLIN|EPOLLRDHUP|EPOLLERR;
    if(_is_et_conn){
        flag |= EPOLLET;
    }
    _connect_fd.set_non_block();
    if(!(_epoll_fd->add_fd(_connect_fd._socket_fd, flag))){
        LOG_ERROR("Server::accept()中返回_epoll_fd->add_fd错误")
        return 1;
    }else{
        return 0;
    }
}

int Server::stop() {
    _close = 1;
    _listen_fd.close();
    return 0;
}
}