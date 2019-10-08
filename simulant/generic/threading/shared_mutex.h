/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../atomic.h"

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

namespace smlt {

class shared_mutex {
public:
    shared_mutex() = default;
    shared_mutex(const shared_mutex& rhs) = delete;
    shared_mutex& operator=(const shared_mutex& rhs) = delete;

    void lock() {
        global_lock_.lock(); // Writer lock
    }

    void unlock() {
        global_lock_.unlock(); // Writer unlock
    }

    void lock_shared() {
        std::lock_guard<std::mutex> g(read_lock_);
        if(++counter_ == 1) {
            global_lock_.lock();
        }
    }

    void unlock_shared() {
        std::lock_guard<std::mutex> g(read_lock_);
        if(--counter_ == 0) {
            global_lock_.unlock();
        }
    }

private:
    atomic<int32_t> counter_ = {0};
    std::mutex read_lock_;
    std::mutex global_lock_;
};

template<typename Mutex>
class write_lock;

template<>
class write_lock<shared_mutex> {
public:
    write_lock(shared_mutex& mutex):
        mutex_(mutex) {
        mutex_.lock();
    }

    ~write_lock() {
        mutex_.unlock();
    }

private:
    shared_mutex& mutex_;
};

template<typename Mutex>
class read_lock;

template<>
class read_lock<shared_mutex> {
public:
    read_lock(shared_mutex& mutex):
        mutex_(mutex) {

        mutex_.lock_shared();
    }

    ~read_lock() {
        mutex_.unlock_shared();
    }

private:
    shared_mutex& mutex_;
};

}
