#pragma once

#include <pthread.h>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <cstdlib>
#include <functional>
#include <memory>
#include <cassert>
#include <utility>

#include "../utils/string.h"
#include "../macros.h"

namespace smlt {
namespace thread {

template< class T >
using decay_t = typename std::decay<T>::type;

typedef uint32_t ThreadID;

class ThreadSpawnError: public std::runtime_error {
public:
    ThreadSpawnError(int code):
        std::runtime_error("Error spawning thread: " + smlt::to_string(code)) {
    }
};


class CallableWrapperBase {
public:
    inline virtual ~CallableWrapperBase() {}

    virtual void call() = 0;
};

template <typename Callable>
class CallableWrapper : public CallableWrapperBase {
    Callable callable_;
public:
    CallableWrapper(Callable&& callable):
        callable_(std::forward<Callable>(callable)) {}

    void call() override {
        callable_();
    }
};

class Thread {
public:
    Thread(const Thread&) = delete;

    template<typename Func, typename... Args>
    explicit Thread(Func&& function, Args&&... args) {
        CallableWrapperBase* func = make_routine(
            std::bind<void>(
                std::forward<Func>(function), std::forward<Args>(args)...
            )
        );

        auto ret = pthread_create(&thread_, NULL, &Thread::thread_runner, func);
        if(ret) {
            throw ThreadSpawnError(ret);
        }
    }

    template<typename Callable>
    CallableWrapperBase* make_routine(Callable&& f) {
        return new CallableWrapper<Callable>(std::forward<Callable>(f));
    }

    void join();
    bool joinable() const;
    void detach();

    static void exit();

    ThreadID id() const {
        /* NOTE: This assumes that pthread_t is implicitly convertible
         * to an unsigned int without losing information. On the Dreamcast
         * this is a kthread_t* but pointers are 32 bit, so that's "ok".
         */
#ifndef _arch_dreamcast
        static_assert(std::is_convertible<pthread_t, ThreadID>::value, "pthread_t is not convertible");
#endif
        return (ThreadID) thread_;
    }

private:
    typedef std::function<void ()> Callable;
    static void* thread_runner(void* data);
    pthread_t thread_ = 0;
};

void yield();
void sleep(size_t ms);

ThreadID this_thread_id();

}
}
