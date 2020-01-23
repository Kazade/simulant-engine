#include "mutex.h"

namespace smlt {
namespace thread {

Mutex::Mutex() {
    pthread_mutex_init(&mutex_, NULL);
}

Mutex::~Mutex() {
    pthread_mutex_destroy(&mutex_);
}

RecursiveMutex::RecursiveMutex() {
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_, &attr);
    pthread_mutexattr_destroy(&attr);
}

RecursiveMutex::~RecursiveMutex() {
    pthread_mutex_destroy(&mutex_);
}

}
}
