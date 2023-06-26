#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <assert.h>

namespace wut::zgy::cppnetwork{
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8): _pool(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        for(size_t i = 0; i < threadCount; i++) {
            std::thread([pool = _pool] {
                std::unique_lock<std::mutex> locker(pool->_mtx);
                while(true) {
                    if(!pool->_tasks.empty()) {
                        auto task = std::move(pool->_tasks.front());
                        pool->_tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if(pool->_is_closed) break;
                    else pool->_cond.wait(locker);
                }
            }).detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool() {
        if(static_cast<bool>(_pool)) {
            {
                std::lock_guard<std::mutex> locker(_pool->_mtx);
                _pool->_is_closed = true;
            }
            _pool->_cond.notify_all();
        }
    }

    template<class F>
    void add_task(F&& task) {
        {
            std::lock_guard<std::mutex> locker(_pool->_mtx);
            _pool->_tasks.emplace(std::forward<F>(task));
        }
        _pool->_cond.notify_one();
    }

private:
    struct Pool {
        std::mutex _mtx;
        std::condition_variable _cond;
        bool _is_closed;
        std::queue<std::function<void()>> _tasks;
    };
    std::shared_ptr<Pool> _pool;
};

}