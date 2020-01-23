
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
    Callable* func = reinterpret_cast<Callable*>(data);
    assert(func);
    if(func && *func) {
        Callable& call = *func;
        call();
        delete func;
    }

    return NULL;
}

void yield() {
    pthread_yield();
}

}
}
