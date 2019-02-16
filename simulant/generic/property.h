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

#include <functional>
#include <memory>
#include <cstdint>

#include "default_init_ptr.h"

namespace smlt {

template<typename Container, typename T, bool IsFunction=false>
class Property {};

template<typename Container, typename T>
class Property<Container, T, true> {
public:
    Property(Container* _this, std::function<T* (Container*)> getter):
        this_(_this),
        getter_(getter) {

    }

    /*
     *  We can't allow copy construction, because 'this_' will never be initialized
     */
    Property(const Property& rhs) = delete;

    Property operator=(const Property& rhs) {
        assert(this_); //Make sure that this_ was initialized

        getter_ = rhs.getter_;
        // Intentionally don't transfer 'this_'
    }

    inline operator T&() const { return *getter_(this_); }
    inline T* operator->() const { return getter_(this_); }

    // Implicit conversion to a T*
    inline operator T*() { return getter_(this_); }

    T* get() const { return getter_(this_); }

    operator bool() const {
        return bool(get());
    }
private:
    Container* this_ = nullptr;

    std::function<T* (Container*)> getter_;
};


template<typename Container, typename T>
class Property<Container, T, false> {
public:
    typedef Property<Container, T, false> this_type;

    Property(Container* _this, T Container::* member):
        this_(_this),
        type_(VALUE),
        value_member(member) {}

    Property(Container* _this, smlt::default_init_ptr<T> Container::* member):
        this_(_this),
        type_(INIT_PTR),
        init_ptr_member(member) {}

    Property(Container* _this, T* Container::* member):
        this_(_this),
        type_(POINTER),
        pointer_member(member) {}

    Property(Container* _this, std::unique_ptr<T> Container::* member):
        this_(_this),
        type_(UNIQUE_PTR),
        unique_ptr_member(member) {}

    Property(Container* _this, std::shared_ptr<T> Container::* member):
        this_(_this),
        type_(SHARED_PTR),
        shared_ptr_member(member) {}

    /*
     *  We can't allow copy construction, because 'this_' will never be initialized
     */
    Property(const this_type& rhs) = delete;
    Property& operator=(const this_type& rhs) {
        assert(this_); //Make sure that this_ was initialized

        // Intentionally don't transfer 'this_'
        type_ = rhs.type_;
        switch(type_) {
            case VALUE: value_member = rhs.value_member; break;
            case POINTER: pointer_member = rhs.pointer_member; break;
            case INIT_PTR: init_ptr_member = rhs.init_ptr_member; break;
            case SHARED_PTR: shared_ptr_member = rhs.shared_ptr_member; break;
            case UNIQUE_PTR: unique_ptr_member = rhs.unique_ptr_member; break;
        default:
            break;
        }

        return *this;
    }

    inline operator T&() const { return *get(); }
    inline T* operator->() const { return get(); }

    // Implicit conversion to a T*
    inline operator T*() { return get(); }

    T* get() const {
        switch(type_) {
        case VALUE:
            return &(this_->*value_member);
        case POINTER:
            return (this_->*pointer_member);
        case INIT_PTR:
            return (this_->*init_ptr_member);
        case SHARED_PTR:
            return (this_->*shared_ptr_member).get();
        case UNIQUE_PTR:
            return (this_->*unique_ptr_member).get();
        }
        return nullptr;
    }

    operator bool() const {
        return bool(get());
    }

private:
    Container* this_ = nullptr;

    enum SourceType : uint8_t {
        POINTER,
        VALUE,
        INIT_PTR,
        SHARED_PTR,
        UNIQUE_PTR
    };

    SourceType type_;
    union {
        T* Container::* pointer_member;
        T Container::* value_member;
        smlt::default_init_ptr<T> Container::* init_ptr_member;
        std::shared_ptr<T> Container::* shared_ptr_member;
        std::unique_ptr<T> Container::* unique_ptr_member;
    };
};

}
