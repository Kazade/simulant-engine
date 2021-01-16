#pragma once

#include <stdexcept>

#ifdef __PSP__
#include <pspthreadman.h>
#else
#include "pthread.h"
#endif

namespace smlt {
namespace thread {

class MutexInitialisationError:
    public std::runtime_error {

public:
    MutexInitialisationError():
        std::runtime_error("Mutex initialisation failed") {}
};

class Mutex {
public:
    friend class Condition;

    Mutex();
    ~Mutex();

    Mutex(const Mutex&) = delete;

    bool try_lock();
    void lock();
    void unlock();

private:
#ifdef __PSP__
    SceUID semaphore_;
    uint32_t owner_ = 0;
#else
    pthread_mutex_t mutex_;
#endif
};

class RecursiveMutex {
public:
    friend class Condition;

    RecursiveMutex();
    ~RecursiveMutex();

    RecursiveMutex(const RecursiveMutex&) = delete;

    void lock();
    void unlock();
private:
#ifdef __PSP__
    SceUID semaphore_;
    uint32_t owner_ = 0;
    int32_t recursive_ = 0;
#else
    pthread_mutex_t mutex_;
#endif
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

}
}
