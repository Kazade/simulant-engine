#pragma once

#include <pthread.h>
#include <cstdint>
#include <stdexcept>
#include <cstdlib>
#include <functional>
#include <memory>
#include <cassert>

namespace smlt {
namespace thread {


typedef uint32_t ThreadID;

class ThreadSpawnError: public std::runtime_error {
public:
    ThreadSpawnError(int code):
        std::runtime_error("Error spawning thread") {

    }
};

class Thread {
public:
    Thread(const Thread&) = delete;

    template<typename Func>
    explicit Thread(Func&& function) {
        auto lambda = [function]{ function(); };

        auto func = new Callable(lambda);

        auto ret = pthread_create(&thread_, NULL, &Thread::thread_runner, func);
        if(ret) {
            throw ThreadSpawnError(ret);
        }
    }

    template<typename Func, typename... Args>
    explicit Thread(Func&& function, Args&&... args) {

        auto f = std::bind(std::forward<Func>(function), std::forward<Args>(args)...);
        auto lambda = [f]{ f(); };
        auto func = std::make_shared<Callable> (
            lambda
        );

        auto ret = pthread_create(&thread_, NULL, &Thread::thread_runner, func);
        if(ret) {
            throw ThreadSpawnError(ret);
        }
    }

    void join();
    bool joinable() const;
    void detach();

    ThreadID id() const {
        return thread_;
    }

private:
    static void* thread_runner(void* data);

    typedef std::function<void()> Callable;
    pthread_t thread_ = 0;
};

void yield();

}
}
