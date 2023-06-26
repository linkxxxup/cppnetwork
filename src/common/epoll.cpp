#include "epoll.h"

namespace wut::zgy::cppnetwork{
    Epoller::Epoller(int maxEvent):_epoll_fd(epoll_create(512)), _max_event(maxEvent){
        assert(_epoll_fd >= 0 );
        _events = new epoll_event[_max_event];
    }

    Epoller::~Epoller() {
        delete[] _events;
        _events = nullptr;
        close();
    }

    bool Epoller::add_fd(int fd, uint32_t events) {
        if(fd < 0) return false;
        epoll_event ev = {0};
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    }

    bool Epoller::mod_fd(int fd, uint32_t events) {
        if(fd < 0) return false;
        epoll_event ev = {0};
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    }

    bool Epoller::del_fd(int fd, uint32_t events) {
        if(fd < 0) return false;
        epoll_event ev = {0};
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &ev);
    }

    void Epoller::close() {
        ::close(_epoll_fd);
    }

    int Epoller::wait(int timeout_ms) {
        return epoll_wait(_epoll_fd, _events, _max_event, timeout_ms);
    }

    int Epoller::get_event_fd(size_t i) const {
        assert(i < _max_event && i >= 0);
        return _events[i].data.fd;
    }

    uint32_t Epoller::get_events(size_t i) const {
        assert(i < _max_event && i >= 0);
        return _events[i].events;
    }
}
