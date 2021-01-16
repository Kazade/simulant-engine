#include <cassert>
#include "condition.h"
#include "../compat.h"

namespace smlt {
namespace thread {

Condition::Condition() {
#ifdef __PSP__
    wait_sem_ = sceKernelCreateSema(
        std::to_string((int) rand()).c_str(),
        0, 0, 1, 0
    );

    wait_done_= sceKernelCreateSema(
        std::to_string((int) rand()).c_str(),
        0, 0, 1, 0
    );
#else
    pthread_cond_init(&cond_, NULL);
#endif
}

Condition::~Condition() {
#ifdef __PSP__
    sceKernelDeleteSema(wait_sem_);
    sceKernelDeleteSema(wait_done_);
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
#else
    assert(!mutex.try_lock());  /* Mutex should've been locked by this thread */
    pthread_cond_wait(&cond_, &mutex.mutex_); /* FIXME: I've heard that this can wake early? */
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
#else
    pthread_cond_signal(&cond_);
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
#else
    pthread_cond_broadcast(&cond_);
#endif
}

}
}
