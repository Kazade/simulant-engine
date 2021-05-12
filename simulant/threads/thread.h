#pragma once

#ifdef __PSP__
#include <pspthreadman.h>
#elif defined(__DREAMAST__)
#include <kos/thread.h>
#else
#include <pthread.h>
#endif

#include <string>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <cstdlib>
#include <functional>
#include <memory>
#include <cassert>
#include <utility>
#include <string>
#include <iostream>

#include "../compat.h"
#include "../errors.h"
#include "../macros.h"

namespace smlt {
namespace thread {

template< class T >
using decay_t = typename std::decay<T>::type;

#if defined(__PSP__) || defined(__DREAMCAST__)
typedef uint32_t ThreadID;
#else
typedef uint64_t ThreadID;
#endif

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
    ~Thread();

    template<typename Func, typename... Args>
    explicit Thread(Func&& function, Args&&... args) {
        CallableWrapperBase* func = make_routine(
            std::bind<void>(
                std::forward<Func>(function), std::forward<Args>(args)...
            )
        );

#ifdef __PSP__
        /* We use the current thread's priority for the new thread */
        int priority = sceKernelGetThreadCurrentPriority();
        thread_ = sceKernelCreateThread(
            smlt::to_string(rand()).c_str(),
            &Thread::thread_runner, priority, 0x10000,
            0, NULL
        );

        sceKernelStartThread(thread_, sizeof(func), func);
#elif defined(__DREAMCAST__)

        /* The default stack size is 32k, which is a bit low for us */
        kthread_attr_t attr = {0};
        attr.stack_size = 256 * 1024;

        thread_ = thd_create_ex(&attr, &Thread::thread_runner, func);
        if(!thread_) {
            FATAL_ERROR(ERROR_CODE_THREAD_SPAWN_FAILED, "Unable to create thread");
        }
#else
        auto ret = pthread_create(&thread_, NULL, &Thread::thread_runner, func);
        if(ret) {
            FATAL_ERROR(ERROR_CODE_THREAD_SPAWN_FAILED, "Unable to create thread");
        }
#endif
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
#if !defined(__DREAMCAST__) && !defined(__APPLE__) && !defined(__PSP__)
        static_assert(std::is_convertible<pthread_t, ThreadID>::value, "pthread_t is not convertible");
#endif
        return (ThreadID) thread_;
    }

private:
    typedef std::function<void ()> Callable;

#ifdef __PSP__
    static int thread_runner(unsigned int, void* data);
    int thread_ = 0;
#elif defined(__DREAMCAST__)
    static void* thread_runner(void* data);
    kthread_t* thread_ = nullptr;
#else
    static void* thread_runner(void* data);
    pthread_t thread_ = 0;
#endif
};

void yield();
void sleep(size_t ms);

ThreadID this_thread_id();

}
}
