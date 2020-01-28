#include <pthread.h>
#include <cassert>
#include "mutex.h"
#include "../macros.h"

namespace smlt {
namespace thread {

Mutex::Mutex() {
    if(pthread_mutex_init(&mutex_, NULL) != 0) {
        throw MutexInitialisationError();
    }
}

Mutex::~Mutex() {
    int err = pthread_mutex_destroy(&mutex_);
    _S_UNUSED(err);
    assert(!err);
}

RecursiveMutex::RecursiveMutex() {
    int err = 0;

#ifdef _arch_dreamcast
    /* Annoyingly pthreads on the Dreamcast are currently missing
     * recursive mutexes. Need to send a patch upstream. Fortunately pthread_mutex_t
     * is just mutex_t so we can use raw KOS functions instead */

    err = mutex_init(&mutex_, MUTEX_TYPE_RECURSIVE);
#else
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    err = pthread_mutex_init(&mutex_, &attr);
    pthread_mutexattr_destroy(&attr);
#endif

    if(err != 0) {
        throw MutexInitialisationError();
    }
}

RecursiveMutex::~RecursiveMutex() {
    int err = 0;
#ifdef _arch_dreamcast
    err = mutex_destroy(&mutex_);
#else
    err = pthread_mutex_destroy(&mutex_);
#endif
    _S_UNUSED(err);
    assert(!err);
}

void RecursiveMutex::lock() {
#ifdef _arch_dreamcast
    mutex_lock(&mutex_);
#else
    pthread_mutex_lock(&mutex_);
#endif
}

void RecursiveMutex::unlock() {
#ifdef _arch_dreamcast
    mutex_unlock(&mutex_);
#else
    pthread_mutex_unlock(&mutex_);
#endif
}

}
}
