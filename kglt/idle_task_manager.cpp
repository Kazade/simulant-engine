#include <utility>
#include <tr1/functional>
#include "kazbase/logging.h"
#include "idle_task_manager.h"

namespace kglt {

static ConnectionID connection_counter = 0;

IdleTaskManager::IdleTaskManager() {

}

ConnectionID IdleTaskManager::add(std::tr1::function<bool ()> callback) {
    ConnectionID new_id = ++connection_counter;
    signals_.insert(std::make_pair(new_id, callback));
    return new_id;
}

ConnectionID IdleTaskManager::add_once(std::tr1::function<void ()> callback) {
    ConnectionID new_id = ++connection_counter;
    signals_once_.insert(std::make_pair(new_id, callback));
    return new_id;
}

void IdleTaskManager::execute() {
    std::map<ConnectionID, std::tr1::function<bool ()> > signals = signals_;
    
    std::map<ConnectionID, std::tr1::function<bool ()> >::iterator it = signals.begin();
    
    for(; it != signals.end(); ++it) {
        L_DEBUG("Executing idle task");
        bool result = (*it).second();
        if(!result) {
            L_DEBUG("Idle task returned false. Removing.");
            signals_.erase((*it).first);
        }
    }
    
    for(std::pair<ConnectionID, std::tr1::function<void ()> > p: signals_once_) {
        L_DEBUG("Executing one-off idle task");
        p.second();
    }
    signals_once_.clear();
}

}
