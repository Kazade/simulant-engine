#pragma once

#include <stdexcept>

#ifdef __PSP__
#include <pspthreadman.h>
#elif defined(__DREAMCAST__)
#include <kos/thread.h>
#include <kos/mutex.h>
#elif defined(_MSC_VER)
#include <stdbool.h>
#include <windows.h>
#else
#include "pthread.h"
#endif

namespace smlt {
namespace thread {

#if defined(_MSC_VER)
    typedef CRITICAL_SECTION pthread_mutex_t;
    typedef void pthread_mutexattr_t;
    typedef void pthread_condattr_t;
    typedef void pthread_rwlockattr_t;
    typedef HANDLE pthread_t;
    typedef CONDITION_VARIABLE pthread_cond_t;

    typedef struct {
        SRWLOCK lock;
        bool exclusive;
    } pthread_rwlock_t;

    struct timespec {
        long tv_sec;
        long tv_nsec;
    };
#endif

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
#elif defined(__DREAMCAST__)
    mutex_t mutex_;
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
#elif defined(__DREAMCAST__)
    mutex_t mutex_;
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
