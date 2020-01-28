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

#include "atomic.h"

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
namespace thread {

class SharedMutex {
public:
    SharedMutex() = default;
    SharedMutex(const SharedMutex& rhs) = delete;
    SharedMutex& operator=(const SharedMutex& rhs) = delete;

    void lock() {
        global_lock_.lock(); // Writer lock
    }

    void unlock() {
        global_lock_.unlock(); // Writer unlock
    }

    void lock_shared() {
        thread::Lock<thread::Mutex> g(read_lock_);
        if(++counter_ == 1) {
            global_lock_.lock();
        }
    }

    void unlock_shared() {
        thread::Lock<thread::Mutex> g(read_lock_);
        if(--counter_ == 0) {
            global_lock_.unlock();
        }
    }

private:
    Atomic<int32_t> counter_ = {0};
    thread::Mutex read_lock_;
    thread::Mutex global_lock_;
};

template<typename Mutex>
class WriteLock;

template<>
class WriteLock<SharedMutex> {
public:
    WriteLock(SharedMutex& mutex):
        mutex_(mutex) {
        mutex_.lock();
    }

    ~WriteLock() {
        mutex_.unlock();
    }

private:
    SharedMutex& mutex_;
};

template<typename Mutex>
class ReadLock;

template<>
class ReadLock<SharedMutex> {
public:
    ReadLock(SharedMutex& mutex):
        mutex_(mutex) {

        mutex_.lock_shared();
    }

    ~ReadLock() {
        mutex_.unlock_shared();
    }

private:
    SharedMutex& mutex_;
};

}
}
