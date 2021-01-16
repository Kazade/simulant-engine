
#ifndef __PSP__
#include <pthread.h>
#endif

#include <cassert>
#include <cstdlib>

#include "mutex.h"
#include "thread.h"

#include "../macros.h"
#include "../errors.h"
#include "../compat.h"

namespace smlt {
namespace thread {

#ifdef __PSP__

static int await_semaphore(SceUID sem) {
    /* I'm hoping this decrements 1! */
    sceKernelWaitSema(sem, 1, NULL);
    return 0;
}

static int post_semaphore(SceUID sem) {
    sceKernelSignalSema(sem, 1);
    return 0;
}
#endif

Mutex::Mutex() {
#ifdef __PSP__
    /* Create a semaphore with an initial and max value of 1 */
    semaphore_ = sceKernelCreateSema(
        std::to_string((int) rand()).c_str(),
        0, 1, 1, 0
    );
    owner_ = 0; /* No owner */
#else
    if(pthread_mutex_init(&mutex_, NULL) != 0) {
        FATAL_ERROR(ERROR_CODE_MUTEX_INIT_FAILED, "Failed to create mutex");
    }
#endif
}

Mutex::~Mutex() {

#ifdef __PSP__
    sceKernelDeleteSema(semaphore_);
#else
#ifndef NDEBUG
    /* This checks to make sure that the mutex
     * was unlocked, if it was locked this would return
     * non-zero, destroying a locked mutex is undefined
     * behaviour */
    assert(!pthread_mutex_trylock(&mutex_));

    /* If it was unlocked, it will now be locked, so
     * unlock it again */
    pthread_mutex_unlock(&mutex_);
#endif

    int err = pthread_mutex_destroy(&mutex_);
    _S_UNUSED(err);
    assert(!err);
#endif
}

bool Mutex::try_lock() {
#ifdef __PSP__
    auto res = sceKernelPollSema(semaphore_, 1);
    if (res < 0) {
        return false;
    }
    return true;
#else
    return pthread_mutex_trylock(&mutex_) == 0;
#endif
}

void Mutex::lock() {
#ifdef __PSP__
    auto tid = this_thread_id();
    await_semaphore(semaphore_);
    owner_ = tid;
#else
    pthread_mutex_lock(&mutex_);
#endif
}

void Mutex::unlock() {
#ifdef __PSP__
    if(this_thread_id() != owner_) {
        return;
    }

    owner_ = 0;
    post_semaphore(semaphore_);
#else
    pthread_mutex_unlock(&mutex_);
#endif
}

RecursiveMutex::RecursiveMutex() {
    int err = 0;

#ifdef __PSP__
    /* Create a semaphore with an initial and max value of 1 */
    semaphore_ = sceKernelCreateSema(
        std::to_string((int) rand()).c_str(),
        0, 1, 1, 0
    );
    owner_ = 0; /* No owner */
    recursive_ = 0;

#elif defined(__DREAMCAST__)
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
        FATAL_ERROR(
            ERROR_CODE_MUTEX_INIT_FAILED,
            "Failed to initialize recursive mutex"
        );
    }
}

RecursiveMutex::~RecursiveMutex() {
    int err = 0;
#ifdef __PSP__
    sceKernelDeleteSema(semaphore_);
#elif defined(__DREAMCAST__)
    err = mutex_destroy(&mutex_);
#else
    err = pthread_mutex_destroy(&mutex_);
#endif
    _S_UNUSED(err);
    assert(!err);
}

void RecursiveMutex::lock() {
#ifdef __PSP__
    auto tid = this_thread_id();
    if(owner_ == tid) {
        ++recursive_;
    } else {
        await_semaphore(semaphore_);
        owner_ = tid;
        recursive_ = 0;
    }
#elif defined(__DREAMCAST__)
    mutex_lock(&mutex_);
#else
    pthread_mutex_lock(&mutex_);
#endif
}

void RecursiveMutex::unlock() {
#ifdef __PSP__
    if(this_thread_id() != owner_) {
        return;
    }

    if(recursive_) {
        recursive_--;
    } else {
        owner_ = 0;
        post_semaphore(semaphore_);
    }
#elif defined(__DREAMCAST__)
    mutex_unlock(&mutex_);
#else
    pthread_mutex_unlock(&mutex_);
#endif
}

}
}
