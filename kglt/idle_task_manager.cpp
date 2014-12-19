#include <utility>
#include <functional>
#include "kazbase/logging.h"
#include "idle_task_manager.h"
#include "window_base.h"
#include "utils/gl_thread_check.h"

namespace kglt {

static ConnectionID connection_counter = 0;

IdleTaskManager::IdleTaskManager(WindowBase &window):
    window_(window) {

}

ConnectionID IdleTaskManager::add(std::function<bool ()> callback) {
    std::lock_guard<std::mutex> lock(signals_mutex_);

    ConnectionID new_id = ++connection_counter;
    signals_.insert(std::make_pair(new_id, callback));
    return new_id;
}

ConnectionID IdleTaskManager::add_once(std::function<void ()> callback) {
    std::lock_guard<std::mutex> lock(signals_once_mutex_);

    ConnectionID new_id = ++connection_counter;
    signals_once_.insert(std::make_pair(new_id, callback));
    return new_id;
}

void IdleTaskManager::run_sync(std::function<void()> callback) {
    /*
     *  If the current thread is not the main thread, then add the task to idle, and
     *  don't return until it runs. Otherwise, run the function immediately.
     */

    if(GLThreadCheck::is_current()) { //Shouldn't abuse the GL thread check like this really but GL thread == main
        callback();
    } else {
        add_once(callback);
        wait();
    }
}

struct TimedTrigger {
    TimedTrigger(float time, std::function<void ()> callback):
        timeout_(time),
        callback_(callback) {
    }

    bool update(kglt::WindowBase* window) {
        auto now = window->total_time();
        if(!start_time_) {
            start_time_ = now;
        }

        if((now - start_time_) > timeout_) {
            callback_();
            return false;
        }
        return true;
    }

    float timeout_ = 0.0;
    float start_time_ = 0.0;
    std::function<void ()> callback_;
};

ConnectionID IdleTaskManager::add_timeout(float seconds, std::function<void()> callback) {
    TimedTrigger trigger(seconds, callback);
    return add(std::bind(&TimedTrigger::update, trigger, &this->window_));
}

void IdleTaskManager::wait() {
    std::unique_lock<std::mutex> lk(cv_mutex_);
    cv_.wait(lk);
}

void IdleTaskManager::execute() {
    {
        //FIXME: If (*it).second tries to queue on idle this will deadlock
        std::lock_guard<std::mutex> lock(signals_mutex_);
        for(auto it = signals_.begin(); it != signals_.end();) {
            bool result = (*it).second();
            if(!result) {
                L_DEBUG("Idle task returned false. Removing.");
                it = signals_.erase(it);
            } else {
                ++it;
            }
        }
    }

    SignalOnceMap to_iter; //Create an empty map
    {
        //Lock the signals once mutex and then swap to clear
        std::lock_guard<std::mutex> lock(signals_once_mutex_);
        std::swap(to_iter, signals_once_);
    }

    //Iterate over the copy
    for(auto p: to_iter) {
        p.second();
    }

    cv_.notify_all(); //Unblock any threads waiting
}

void IdleTaskManager::remove(ConnectionID connection) {
    std::lock_guard<std::mutex> lock1(signals_mutex_);
    std::lock_guard<std::mutex> lock2(signals_once_mutex_);

    signals_.erase(connection);
    signals_once_.erase(connection);
}

}
