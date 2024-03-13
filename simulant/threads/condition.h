#pragma once

#include "mutex.h"

namespace smlt {
namespace thread {

class Condition {
public:
    Condition();
    Condition(const Condition&) = delete;
    Condition& operator=(const Condition&) = delete;
    ~Condition();

    void wait(Mutex& mutex);
    void notify_one();
    void notify_all();

private:
#ifdef __PSP__
    Mutex lock_;
    SceUID wait_sem_;
    SceUID wait_done_;
    int waiting_ = 0;
    int signals_ = 0;
#elif defined(__PS2__)
    Mutex lock_;
    int wait_sem_;
    int wait_done_;
    int waiting_ = 0;
    int signals_ = 0;
#elif defined(__DREAMCAST__)
    condvar_t cond_;
#else
    pthread_cond_t cond_;
#endif
};


}
}
