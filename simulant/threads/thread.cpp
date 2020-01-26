
#ifdef __WIN32__
#include <windows.h>
#else
#include <time.h>
#endif

#include "thread.h"

namespace smlt {
namespace thread {

void Thread::join() {
    pthread_join(thread_, NULL);
}

bool Thread::joinable() const {
    return thread_ > 0;
}

void Thread::detach() {
    pthread_detach(thread_);
    thread_ = 0;
}

void* Thread::thread_runner(void* data) {
    CallableWrapperBase* func = reinterpret_cast<CallableWrapperBase*>(data);
    assert(func);
    if(func) {
        func->call();
        delete func;
    }

    return NULL;
}

void sleep(size_t ms) {
#ifdef __WIN32__
    Sleep(ms);
#else
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = ms * 1000000;
    nanosleep(&tim , &tim2);
#endif
}

void yield() {
    pthread_yield();
}

ThreadID this_thread_id() {
    return pthread_self();
}

}
}
