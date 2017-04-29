/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IDLE_TASK_MANAGER_H
#define IDLE_TASK_MANAGER_H

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <condition_variable>

#include "types.h"

namespace smlt {

class WindowBase;

class IdleTaskManager {
public:
    IdleTaskManager(WindowBase& window);

    IdleConnectionID add(std::function<bool ()> callback);
    IdleConnectionID add_once(std::function<void ()> callback);
    IdleConnectionID add_timeout(float seconds, std::function<void()> callback);
    
    void run_sync(std::function<void()> callback);

    void remove(IdleConnectionID connection);

    void execute();

    void wait();

private:
    typedef std::map<IdleConnectionID, std::function<bool ()> > SignalMap;
    typedef std::map<IdleConnectionID, std::function<void ()> > SignalOnceMap;

    WindowBase& window_;

    SignalMap signals_;
    SignalOnceMap signals_once_;

    std::mutex signals_mutex_;
    std::mutex signals_once_mutex_;

    std::mutex cv_mutex_;
    std::condition_variable cv_;
};

}
#endif // IDLE_TASK_MANAGER_H
