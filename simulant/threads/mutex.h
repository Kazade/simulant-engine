#pragma once

#include "pthread.h"

namespace smlt {
namespace thread {

class Mutex {
public:
    Mutex();
    ~Mutex();

    Mutex(const Mutex&) = delete;

    void lock() {
        pthread_mutex_lock(&mutex_);
    }

    void unlock() {
        pthread_mutex_unlock(&mutex_);
    }

private:
    pthread_mutex_t mutex_;
};

class RecursiveMutex {
public:
    RecursiveMutex();
    ~RecursiveMutex();

    RecursiveMutex(const RecursiveMutex&) = delete;

    void lock() {
        pthread_mutex_lock(&mutex_);
    }

    void unlock() {
        pthread_mutex_unlock(&mutex_);
    }
private:
    pthread_mutex_t mutex_;
};

template<typename M>
class Lock {
public:
    explicit Lock(Mutex& m):
        mutex_(m) {

        mutex_.lock();
    }

    ~Lock() {
        mutex_.unlock();
    }

private:
    M& mutex_;
};

}
}
