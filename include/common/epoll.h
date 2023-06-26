#pragma once

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

namespace wut::zgy::cppnetwork{
class Epoller {
public:
    explicit Epoller(int maxEvent = 200);
    ~Epoller();
    bool add_fd(int fd, uint32_t events); // epoll_event的event类型为uint32_t
    bool mod_fd(int fd, uint32_t events);
    bool del_fd(int fd, uint32_t events);
    void close();
    int wait(int timeout_ms = -1);
    int get_event_fd(size_t i) const;
    uint32_t get_events(size_t i) const;

private:
    const int _max_event;
    int _epoll_fd;
    epoll_event* _events;
};
}