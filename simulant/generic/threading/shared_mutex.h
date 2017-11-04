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

#pragma once

#include <mutex>

/*
 * Shoddy implementation of a multi-reader/single-writer lock until
 * I move away from Ubuntu 14.04 and can use shared_mutex without
 * a boost dependency.
 *
 * Usage:
 *
 * shared_mutex mutex;
 *
 * {
 *      write_lock lock(mutex);
 *      ... do writey things..
 * }
 *
 * {
 *      read_lock lock(mutex);
 *      ... do ready things...
 * }
 */

class shared_mutex {
public:
    shared_mutex() = default;
    shared_mutex(const shared_mutex& rhs) = delete;
    shared_mutex& operator=(const shared_mutex& rhs) = delete;

    void lock() {
        global_lock.lock(); // Writer lock
    }

    void unlock() {
        global_lock.unlock(); // Writer unlock
    }

    void lock_shared() {
        read_lock.lock();
        if(++counter == 1) {
            global_lock.lock();
        }
        read_lock.unlock();
    }

    void unlock_shared() {
        read_lock.lock();
        if(--counter == 0) {
            global_lock.unlock();
        }
        read_lock.unlock();
    }

private:
    int32_t counter = 0;
    std::mutex read_lock;
    std::mutex global_lock;
};


template<typename Mutex>
class write_lock {
public:
    write_lock(Mutex& mutex):
        mutex_(mutex) {
        mutex_.lock();
    }

    ~write_lock() {
        mutex_.unlock();
    }

private:
    Mutex& mutex_;
};

template<typename Mutex>
class read_lock {
public:
    read_lock(Mutex& mutex):
        mutex_(mutex) {

        mutex_.lock_shared();
    }

    ~read_lock() {
        mutex_.unlock_shared();
    }

private:
    Mutex& mutex_;
};
