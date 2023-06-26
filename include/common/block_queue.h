#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>
#include <cassert>

namespace wut::zgy::cppnetwork{

template <typename T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity);
    ~BlockDeque();
    void clear();
    bool empty();
    bool full();
    void close();
    size_t size();
    size_t capacity();
    T front();
    T back();
    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    // 增加了超时处理
    bool pop(T &item, int timeout);
    void flush();

private:
    std::deque<T> _deq;
    size_t _capacity;
    std::mutex _mtx;
    bool _is_close;
    std::condition_variable _cond_consumer;
    std::condition_variable _cond_producer;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :_capacity(MaxCapacity) {
    assert(MaxCapacity > 0);
    _is_close = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    close();
};

template<class T>
void BlockDeque<T>::close() {
    {
        std::lock_guard<std::mutex> locker(_mtx);
        _deq.clear();
        _is_close = true;
    }
    _cond_producer.notify_all();
    _cond_consumer.notify_all();
};

template<class T>
void BlockDeque<T>::flush() {
    _cond_consumer.notify_one();
};

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(_mtx);
    _deq.clear();
}

template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.front();
}

template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.back();
}

template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.size();
}

template<class T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _capacity;
}

template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(_mtx);
    while(_deq.size() >= _capacity) {
        _cond_producer.wait(locker);
    }
    _deq.push_back(item);
    _cond_consumer.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(_mtx);
    while(_deq.size() >= _capacity) {
        _cond_producer.wait(locker);
    }
    _deq.push_front(item);
    _cond_consumer.notify_one();
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.empty();
}

template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.size() >= _capacity;
}

template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(_mtx);
    while(_deq.empty()){
        _cond_consumer.wait(locker);
        if(_is_close){
            return false;
        }
    }
    item = _deq.front();
    _deq.pop_front();
    _cond_producer.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(_mtx);
    while(_deq.empty()){
        if(_cond_consumer.wait_for(locker, std::chrono::seconds(timeout))
           == std::cv_status::timeout){
            return false;
        }
        if(_is_close){
            return false;
        }
    }
    item = _deq.front();
    _deq.pop_front();
    _cond_producer.notify_one();
    return true;
}

}