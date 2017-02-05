//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <utility>
#include <functional>
#include "deps/kazlog/kazlog.h"
#include "idle_task_manager.h"
#include "window_base.h"
#include "utils/gl_thread_check.h"

namespace smlt {

static IdleConnectionID connection_counter = 0;

IdleTaskManager::IdleTaskManager(WindowBase &window):
    window_(window) {

}

IdleConnectionID IdleTaskManager::add(std::function<bool ()> callback) {
    std::lock_guard<std::mutex> lock(signals_mutex_);

    IdleConnectionID new_id = ++connection_counter;
    signals_.insert(std::make_pair(new_id, callback));
    return new_id;
}

IdleConnectionID IdleTaskManager::add_once(std::function<void ()> callback) {
    std::lock_guard<std::mutex> lock(signals_once_mutex_);

    IdleConnectionID new_id = ++connection_counter;
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

    bool update(smlt::WindowBase* window) {
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

IdleConnectionID IdleTaskManager::add_timeout(float seconds, std::function<void()> callback) {
    std::shared_ptr<TimedTrigger> trigger(new TimedTrigger(seconds, callback));
    return add(std::bind(&TimedTrigger::update, trigger, &this->window_));
}

void IdleTaskManager::wait() {
    std::unique_lock<std::mutex> lk(cv_mutex_);
    cv_.wait(lk);
}

void IdleTaskManager::execute() {
    {
        SignalMap signals_copy;
        {
            std::lock_guard<std::mutex> lock(signals_mutex_);
            signals_copy = signals_;
        }

        std::vector<IdleConnectionID> to_erase;

        for(auto pair: signals_copy) {
            bool result = pair.second();
            if(!result) {
                L_DEBUG("Idle task returned false. Removing.");
                to_erase.push_back(pair.first);
            }
        }

        {
            std::lock_guard<std::mutex> lock(signals_mutex_);
            for(auto conn: to_erase) {
                signals_.erase(conn);
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

void IdleTaskManager::remove(IdleConnectionID connection) {
    std::lock_guard<std::mutex> lock1(signals_mutex_);
    std::lock_guard<std::mutex> lock2(signals_once_mutex_);

    signals_.erase(connection);
    signals_once_.erase(connection);
}

}
