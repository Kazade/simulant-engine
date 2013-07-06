#include <utility>
#include <tr1/functional>
#include "kazbase/logging.h"
#include "idle_task_manager.h"

namespace kglt {

static ConnectionID connection_counter = 0;

IdleTaskManager::IdleTaskManager() {

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
