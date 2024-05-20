#if !defined(_MSC_VER)
#include <pthread.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#elif defined(__PSP__)
#include <pspsdk.h>
#include <pspthreadman.h>
#else
#include <time.h>
#endif

#include "../logging.h"
#include "thread.h"

namespace smlt {
namespace thread {

Thread::~Thread() {
    S_DEBUG("Thread destructor called");

    if(!thread_) {
        return;
    }

#ifdef __PSP__
    sceKernelDeleteThread(thread_);
#elif defined(__DREAMCAST__)
    thd_destroy(thread_);
#endif
}

void Thread::join() {
    assert(thread_);

    S_DEBUG("Joining thread {0}", this_thread_id());

#ifdef __PSP__
    SceKernelThreadInfo status;
    status.size = sizeof(SceKernelThreadInfo);

    while(true) {
        if(sceKernelReferThreadStatus(thread_, &status) == 0) {
            if(status.status == PSP_THREAD_STOPPED || status.status == PSP_THREAD_KILLED) {
                break;
            } else {
                sleep(10);
            }
        } else {
            FATAL_ERROR(ERROR_CODE_THREAD_JOIN_FAILED, "Unable to get thread status");
        }
    }

    psp_thread_state_.reset();
#elif defined(__DREAMCAST__)
    thd_join(thread_, nullptr);
#elif defined(_MSC_VER)
    WaitForSingleObject(thread_, INFINITE);
    CloseHandle(thread_);
#else
    pthread_join(thread_, nullptr);
#endif

    thread_ = 0;
}

bool Thread::joinable() const {
    return (ThreadID)thread_ > 0;
}

void Thread::detach() {
#ifdef __PSP__
    psp_thread_state_->detached = true;
#elif defined(__DREAMCAST__)
    thd_detach(thread_);
#elif defined(_MSC_VER)
    CloseHandle(thread_);
#else
    pthread_detach(thread_);
#endif
    thread_ = 0;
}

void Thread::exit() {
    S_DEBUG("Exiting thread {0}", this_thread_id());

#ifdef __PSP__
    sceKernelExitThread(0);
#elif defined(__DREAMCAST__)
    int status = 0;
    thd_exit(&status);
#elif defined(_MSC_VER)
    ExitThread(0);
#else
    int status = 0;
    pthread_exit(&status);
#endif

    S_DEBUG("Thread exited");
}

#ifdef __PSP__
int Thread::thread_runner(unsigned int, void* data) {
    std::shared_ptr<PSPThreadState> state = *((std::shared_ptr<PSPThreadState>*)(data));
    CallableWrapperBase* func = state->func;

#else
void* Thread::thread_runner(void* data) {
    CallableWrapperBase* func = reinterpret_cast<CallableWrapperBase*>(data);
#endif

    assert(func);
    if(func) {
        func->call();
        delete func;
    }

#ifdef __PSP__
    if(state->detached) {
        sceKernelExitDeleteThread(0);
    } else {
        sceKernelExitThread(0);
    }
    return 0;
#else
    return nullptr;
#endif
}

void sleep(size_t ms) {
#ifdef __WIN32__
    Sleep(ms);
#else
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = ms * 1000000;

#ifdef __PSP__
    _S_UNUSED(tim2);
    sceKernelDelayThreadCB(1000000 * tim.tv_sec + (tim.tv_nsec / 1000));
#else
    nanosleep(&tim , &tim2);
#endif

#endif
}

void yield() {
#ifndef SIMULANT_HAS_PTHREAD_YIELD
    /*
     * Massive debates around whether this should be
     * sleep(0) or no-op. My money's on sleep(0).
     */
    sleep(0);
#elif defined(__PSP__)
    /* FIXME: For some reason the CMake check thinks pthread_yield exists? */
    sleep(0);
#else
    pthread_yield();
#endif
}

ThreadID this_thread_id() {
#ifdef __PSP__
    return (ThreadID) sceKernelGetThreadId();
#elif defined(__DREAMCAST__)
    auto thd = thd_get_current();
    return (ThreadID) (thd ? thd->tid : 0);
#elif defined(_MSC_VER)
    return (ThreadID) GetCurrentThreadId();
#else
    auto ret = pthread_self();
    return (ThreadID) ret;
#endif
}

}
}
