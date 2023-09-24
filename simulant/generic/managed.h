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

#include <cassert>
#include <stdexcept>
#include <memory>
#include <string>
#include <functional>

#include "../logging.h"

namespace smlt {

template<typename T>
void deleter(T* obj) {
    obj->clean_up();
    delete obj;
}

class TwoPhaseConstructed {
public:
    virtual ~TwoPhaseConstructed() {
        assert(construction_state_ == 2 && "clean_up was not called");
    }

    bool init() {
        assert(construction_state_++ == 0 && "double init call?");
        return on_init();
    }

    void clean_up() {
        assert(construction_state_ < 2 && "clean_up called multiple times");
        assert(construction_state_++ == 1 && "init was not called, but clean_up was");
        return on_clean_up();
    }

private:
    virtual bool on_init() { return true; }
    virtual void on_clean_up() {}

    // Used to verify that things are working correctly
#ifndef NDEBUG
    uint8_t construction_state_ = 0;
#endif
};

template<typename T>
class RefCounted : public virtual TwoPhaseConstructed, public virtual std::enable_shared_from_this<T> {
public:
    typedef std::shared_ptr<T> ptr;
    typedef std::weak_ptr<T> wptr;

    template<typename... Args>
    static typename RefCounted<T>::ptr create(Args&&... args) {
        typename RefCounted<T>::ptr instance = typename RefCounted<T>::ptr(
            new T(std::forward<Args>(args)...),
            std::bind(&deleter<T>, std::placeholders::_1)
        );

        if(!instance->init()) {
            S_ERROR("Failed to initialize object");
            return RefCounted<T>::ptr();
        }
        return instance;
    }

    static typename RefCounted<T>::ptr create() {
        typename RefCounted<T>::ptr instance = typename RefCounted<T>::ptr(
            new T(),
            std::bind(&deleter<T>, std::placeholders::_1)
        );

        if(!instance->init()) {
            S_ERROR("Failed to initialize object");
            return RefCounted<T>::ptr();
        }
        return instance;
    }

protected:
    RefCounted() = default;
    virtual ~RefCounted() {}

    template<typename...Args>
    RefCounted(Args&&...) {}
};

}

