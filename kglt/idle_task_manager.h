#ifndef IDLE_TASK_MANAGER_H
#define IDLE_TASK_MANAGER_H

#include <cstdint>
#include <sigc++/sigc++.h>
#include <tr1/functional>
#include <map>

namespace kglt {

typedef uint32_t ConnectionID;

class IdleTaskManager {
public:
    IdleTaskManager();

    ConnectionID add(std::tr1::function<bool ()> callback);
    ConnectionID add_once(std::tr1::function<void ()> callback);
    
    void remove(ConnectionID connection);

    void execute();

private:
    std::map<ConnectionID, std::tr1::function<bool ()> > signals_;
    std::map<ConnectionID, std::tr1::function<void ()> > signals_once_;

};

}
#endif // IDLE_TASK_MANAGER_H
