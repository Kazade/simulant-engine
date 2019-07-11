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

#include <stdexcept>
#include <memory>
#include <string>
#include <functional>

namespace smlt {

class InstanceInitializationError :
    public std::runtime_error {

public:
    InstanceInitializationError():
        std::runtime_error("Couldn't initialize the instance") {}

    InstanceInitializationError(const std::string& type):
        std::runtime_error(type + " could not be initialized") {

    }
};

template<typename T>
void deleter(T* obj) {
    obj->cleanup();
    delete obj;
}

class TwoPhaseConstructed {
public:
    virtual ~TwoPhaseConstructed() {}

    virtual bool init() { return true; }
    virtual void cleanup() {
#if DEBUG
        assert(!cleaned_up);
        cleaned_up = true;
#endif
    }

#ifdef DEBUG
    bool cleaned_up = false;
#endif
};

template<typename T>
class RefCounted : public virtual TwoPhaseConstructed, public std::enable_shared_from_this<T> {
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
            throw InstanceInitializationError(typeid(T).name());
        }
        return instance;
    }

    static typename RefCounted<T>::ptr create() {
        typename RefCounted<T>::ptr instance = typename RefCounted<T>::ptr(
            new T(),
            std::bind(&deleter<T>, std::placeholders::_1)
        );

        if(!instance->init()) {
            throw InstanceInitializationError(typeid(T).name());
        }
        return instance;
    }

protected:
    RefCounted() = default;
    virtual ~RefCounted() {}

    template<typename...Args>
    RefCounted(Args&&... args) {}
};

}

