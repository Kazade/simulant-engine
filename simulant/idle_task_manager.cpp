//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <utility>
#include <functional>
#include "logging.h"
#include "idle_task_manager.h"
#include "window.h"
#include "utils/gl_thread_check.h"
#include "time_keeper.h"
#include "application.h"

namespace smlt {

static IdleConnectionID connection_counter = 0;

IdleConnectionID IdleTaskManager::add(std::function<bool ()> callback) {
    thread::Lock<thread::Mutex> lock(signals_mutex_);

    IdleConnectionID new_id = ++connection_counter;
    signals_.insert(std::make_pair(new_id, callback));
    return new_id;
}

IdleConnectionID IdleTaskManager::add_once(std::function<void ()> callback) {
    thread::Lock<thread::Mutex> lock(signals_once_mutex_);

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

struct TimedTriggerOnce {
    TimedTriggerOnce(const Seconds& time, std::function<void ()> callback):
        timeout_(time),
        callback_(callback) {
    }

    bool update(TimeKeeper* time_keeper) {
        auto now = time_keeper->total_elapsed_seconds();
        if(!start_time_) {
            start_time_ = now;
        }

        if((now - start_time_) > timeout_.to_float()) {
            callback_();
            return false;
        }
        return true;
    }

    Seconds timeout_ = Seconds(0.0f);
    float start_time_ = 0.0f;
    std::function<void ()> callback_;
};

struct TimedTrigger {
    TimedTrigger(const Seconds& time, std::function<bool ()> callback):
        timeout_(time),
        callback_(callback) {
    }

    bool update(TimeKeeper* time_keeper) {
        auto now = time_keeper->total_elapsed_seconds();
        if(!start_time_) {
            start_time_ = now;
        }

        if((now - start_time_) > timeout_.to_float()) {
            start_time_ = now;
            return callback_();
        }
        return true;
    }

    Seconds timeout_ = Seconds(0.0f);
    float start_time_ = 0.0f;
    std::function<bool ()> callback_;
};

IdleConnectionID IdleTaskManager::add_timeout(const Seconds& seconds, std::function<bool()> callback) {
    std::shared_ptr<TimedTrigger> trigger(new TimedTrigger(seconds, callback));
    return add(std::bind(&TimedTrigger::update, trigger, get_app()->time_keeper.get()));
}

IdleConnectionID IdleTaskManager::add_timeout_once(const Seconds& seconds, std::function<void()> callback) {
    std::shared_ptr<TimedTriggerOnce> trigger(new TimedTriggerOnce(seconds, callback));
    return add(std::bind(&TimedTriggerOnce::update, trigger, get_app()->time_keeper.get()));
}

void IdleTaskManager::wait() {
    thread::Lock<thread::Mutex> guard(cv_mutex_);
    cv_.wait(cv_mutex_);
}

void IdleTaskManager::execute() {
    {
        SignalMap signals_copy;
        {
            thread::Lock<thread::Mutex> lock(signals_mutex_);
            signals_copy = signals_;
        }

        std::vector<IdleConnectionID> to_erase;

        for(auto pair: signals_copy) {
            bool result = pair.second();
            if(!result) {
                S_DEBUG("Idle task returned false. Removing.");
                to_erase.push_back(pair.first);
            }
        }

        {
            thread::Lock<thread::Mutex> lock(signals_mutex_);
            for(auto conn: to_erase) {
                signals_.erase(conn);
            }
        }
    }

    SignalOnceMap to_iter; //Create an empty map
    {
        //Lock the signals once mutex and then swap to clear
        thread::Lock<thread::Mutex> lock(signals_once_mutex_);
        std::swap(to_iter, signals_once_);
    }

    //Iterate over the copy
    for(auto p: to_iter) {
        p.second();
    }

    cv_.notify_all(); //Unblock any threads waiting
}

void IdleTaskManager::remove(IdleConnectionID connection) {
    thread::Lock<thread::Mutex> lock1(signals_mutex_);
    thread::Lock<thread::Mutex> lock2(signals_once_mutex_);

    signals_.erase(connection);
    signals_once_.erase(connection);
}

}
