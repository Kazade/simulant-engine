#include <cassert>
#include "condition.h"
#include "../compat.h"
#include "../macros.h"

namespace smlt {
namespace thread {

Condition::Condition() {
#ifdef __PSP__
    wait_sem_ = sceKernelCreateSema(
        smlt::to_string((int) rand()).c_str(),
        0, 0, 1, 0
    );

    wait_done_= sceKernelCreateSema(
        smlt::to_string((int) rand()).c_str(),
        0, 0, 1, 0
    );
#elif defined(__PS2__)
    ee_sema_t state;
    state.attr = 0;
    state.initial = 0;
    state.max = 1;
    state.option = 0;

    wait_sem_ = CreateSema(&state);
    wait_done_= CreateSema(&state);

#elif defined(__DREAMCAST__)
    cond_init(&cond_);
#else
    int err = pthread_cond_init(&cond_, NULL);
    _S_UNUSED(err);
    assert(!err);
#endif
}

Condition::~Condition() {
#ifdef __PSP__
    sceKernelDeleteSema(wait_sem_);
    sceKernelDeleteSema(wait_done_);
#elif defined(__PS2__)
    DeleteSema(wait_sem_);
    DeleteSema(wait_done_);
#elif defined(__DREAMCAST__)
    cond_destroy(&cond_);
#else
    pthread_cond_destroy(&cond_);
#endif
}

void Condition::wait(Mutex& mutex) {
#ifdef __PSP__
    {
        Lock<Mutex> lock(lock_);
        ++waiting_;
    }

    mutex.unlock();

    sceKernelWaitSema(wait_sem_, 1, NULL);

    {
        Lock<Mutex> lock(lock_);
        if(signals_ > 0) {
            sceKernelSignalSema(wait_done_, 1);
            --signals_;
        }

        --waiting_;
    }

    mutex.lock();
#elif defined(__PS2__)
    {
        Lock<Mutex> lock(lock_);
        ++waiting_;
    }

    mutex.unlock();

    WaitSema(wait_sem_);

    {
        Lock<Mutex> lock(lock_);
        if(signals_ > 0) {
            SignalSema(wait_done_);
            --signals_;
        }

        --waiting_;
    }

    mutex.lock();
#elif defined(__DREAMCAST__)
    int err = cond_wait(&cond_, &mutex.mutex_);
    _S_UNUSED(err);
    assert(!err);
#else
    assert(!mutex.try_lock());  /* Mutex should've been locked by this thread */
    int err = pthread_cond_wait(&cond_, &mutex.mutex_); /* FIXME: I've heard that this can wake early? */
    _S_UNUSED(err);
    assert(!err);
#endif
}

void Condition::notify_one() {
#ifdef __PSP__
    lock_.lock();
    if(waiting_ > signals_) {
        ++signals_;
        sceKernelSignalSema(wait_sem_, 1);
        lock_.unlock();
        sceKernelSignalSema(wait_done_, 1);
    } else {
        lock_.unlock();
    }
#elif defined(__PS2__)
    lock_.lock();
    if(waiting_ > signals_) {
        ++signals_;
        SignalSema(wait_sem_);
        lock_.unlock();
        SignalSema(wait_done_);
    } else {
        lock_.unlock();
    }
#elif defined(__DREAMCAST__)
    int err = cond_signal(&cond_);
    _S_UNUSED(err);
    assert(!err);
#else
    int err = pthread_cond_signal(&cond_);
    _S_UNUSED(err);
    assert(!err);
#endif
}

void Condition::notify_all() {
#ifdef __PSP__
    lock_.lock();
    if(waiting_ > signals_) {
        int num_waiting = (waiting_ - signals_);
        signals_ = waiting_;
        for(int i = 0; i < num_waiting; ++i) {
            sceKernelSignalSema(wait_sem_, 1);
        }

        lock_.unlock();
        for(int i = 0; i < num_waiting; ++i) {
            sceKernelWaitSema(wait_done_, 1, NULL);
        }
    } else {
        lock_.unlock();
    }
#elif defined(__PS2__)
    lock_.lock();
    if(waiting_ > signals_) {
        int num_waiting = (waiting_ - signals_);
        signals_ = waiting_;
        for(int i = 0; i < num_waiting; ++i) {
            SignalSema(wait_sem_);
        }

        lock_.unlock();
        for(int i = 0; i < num_waiting; ++i) {
            WaitSema(wait_done_);
        }
    } else {
        lock_.unlock();
    }
#elif defined(__DREAMCAST__)
    int err = cond_broadcast(&cond_);
    _S_UNUSED(err);
    assert(!err);
#else
    pthread_cond_broadcast(&cond_);
#endif
}

}
}
