
#include "condition.h"

namespace smlt {
namespace thread {

Condition::Condition() {
    pthread_cond_init(&cond_, NULL);
}

Condition::~Condition() {
    pthread_cond_destroy(&cond_);
}

void Condition::wait(Mutex& mutex) {
    pthread_cond_wait(&cond_, &mutex.mutex_);
}

void Condition::notify_one() {
    pthread_cond_signal(&cond_);
}

void Condition::notify_all() {
    pthread_cond_broadcast(&cond_);
}

}
}
