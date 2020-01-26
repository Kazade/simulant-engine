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
    explicit Lock(M& m):
        mutex_(m) {

        mutex_.lock();
    }

    ~Lock() {
        mutex_.unlock();
    }

private:
    M& mutex_;
};

template<typename M>
class ToggleLock;

template<>
class ToggleLock<Mutex> {
public:
    explicit ToggleLock(Mutex& m):
        mutex_(m) {
        mutex_.lock();
    }

    ~ToggleLock() {
        mutex_.unlock();
    }

    void lock() {
        mutex_.lock();
    }

    void unlock() {
        mutex_.unlock();
    }

private:
    Mutex& mutex_;
};

class Condition {
public:
    Condition();

    void wait(ToggleLock<Mutex>& mutex);
    void notify_one();
    void notify_all();
};


}
}
