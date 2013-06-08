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

void IdleTaskManager::execute() {
    {
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

    {
        std::lock_guard<std::mutex> lock(signals_once_mutex_);

        for(auto p: signals_once_) {
            L_DEBUG("Executing one-off idle task");
            p.second();
        }
        signals_once_.clear();
    }
}

void IdleTaskManager::remove(ConnectionID connection) {
    std::lock_guard<std::mutex> lock1(signals_mutex_);
    std::lock_guard<std::mutex> lock2(signals_once_mutex_);

    signals_.erase(connection);
    signals_once_.erase(connection);
}

}
