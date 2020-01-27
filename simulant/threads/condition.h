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
    pthread_cond_t cond_;
};


}
}
