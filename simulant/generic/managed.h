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

#ifndef MANAGED_H
#define MANAGED_H

#include <stdexcept>
#include <memory>
#include <string>
#include <functional>


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

class ManagedBase {
public:
    virtual ~ManagedBase() {}

    virtual bool init() { return true; }
    virtual void cleanup() {}
};

template<typename T>
class Managed : public virtual ManagedBase {
public:
    typedef std::shared_ptr<T> ptr;
    typedef std::weak_ptr<T> wptr;

    template<typename... Args>
    static typename Managed<T>::ptr create(Args&&... args) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(
            new T(std::forward<Args>(args)...),
            std::bind(&deleter<T>, std::placeholders::_1)
        );

        if(!instance->init()) {
            throw InstanceInitializationError(typeid(T).name());
        }
        return instance;
    }

    static typename Managed<T>::ptr create() {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(
            new T(),
            std::bind(&deleter<T>, std::placeholders::_1)
        );

        if(!instance->init()) {
            throw InstanceInitializationError(typeid(T).name());
        }
        return instance;
    }


    bool uses_gc() const { return uses_gc_; }
    void enable_gc(bool value=true) { uses_gc_ = value; }

protected:
    Managed() = default;
    virtual ~Managed() {}

    template<typename...Args>
    Managed(Args&&... args) {}

private:
    bool uses_gc_ = true;
};

#endif // MANAGED_H
