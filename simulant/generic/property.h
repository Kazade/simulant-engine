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

#include <type_traits>
#include <functional>
#include <memory>
#include <cstdint>

#include "default_init_ptr.h"

namespace smlt {

namespace property_impl {

template<typename MP, typename C, typename T>
struct Getter;

template<typename MP, typename C, typename T>
struct Getter<MP, C, std::weak_ptr<T>> {
    MP mp_;

    Getter(MP mp):
        mp_(mp) {

    }

    T* get(C* _this) const {
        if(auto sptr = (_this->*mp_).lock()) {
            return sptr.get();
        } else {
            return nullptr;
        }
    }
};

template<typename MP, typename C, typename T>
struct Getter<MP, C, std::shared_ptr<T>> {
    MP mp_;

    Getter(MP mp):
        mp_(mp) {

    }

    T* get(C* _this) const {
        return (_this->*mp_).get();
    }
};

template<typename MP, typename C, typename T>
struct Getter<MP, C, std::unique_ptr<T>> {
    MP mp_;

    Getter(MP mp):
        mp_(mp) {

    }

    T* get(C* _this) const {
        return (_this->*mp_).get();
    }
};

template<typename MP, typename C, typename T>
struct Getter<MP, C, default_init_ptr<T>> {
    MP mp_;

    Getter(MP mp):
        mp_(mp) {

    }

    T* get(C* _this) const {
        return (_this->*mp_);
    }
};

template<typename MP, typename C, typename T>
struct Getter<MP, C, T*> {
    static_assert(!std::is_function<MP>::value, "MP was a function");

    MP mp_;

    Getter(MP mp):
        mp_(mp) {

    }

    T* get(C* _this) const {
        return (_this->*mp_);
    }
};


template<typename MP, typename C, typename T>
struct Getter<MP, C, T* () const> {
    MP mp_;

    Getter(MP mp):
        mp_(mp) {

    }

    T* get(C* _this) const {
        return (_this->*mp_)();
    }
};

template<typename MP, typename C, typename T>
struct Getter {
    MP mp_;

    static_assert(!std::is_pointer<T>::value, "T was a pointer");
    static_assert(!std::is_function<T>::value, "T was a function");

    Getter(MP mp):
        mp_(mp) {

    }

    T* get(C* _this) const {
        return &(_this->*mp_);
    }
};

}

template<typename T>
struct ExtractType {
    static_assert(!std::is_function<T>::value, "T was a function");

    typedef T type;
};

template<typename T>
struct ExtractType<T*> {
    typedef typename std::remove_pointer<T>::type type;
};

template<typename T>
struct ExtractType<std::weak_ptr<T>> {
    typedef T type;
};

template<typename T>
struct ExtractType<std::shared_ptr<T>> {
    typedef T type;
};

template<typename T>
struct ExtractType<std::unique_ptr<T>> {
    typedef T type;
};

template<typename T>
struct ExtractType<default_init_ptr<T>> {
    typedef T type;
};

template<typename T>
struct ExtractType<T() const> {
    typedef typename std::remove_pointer<T>::type type;
};


template<typename MP>
struct PointerToMemberHelper;


template<typename T, typename U>
struct PointerToMemberHelper<T U::*> {
    typedef U class_type; /* Class */
    typedef T variable_type; /* The source type */

    /* This is the underlying type, which needs extracting */
    typedef typename ExtractType<
        T
    >::type result_type;
};

template<typename MP>
struct PointerToMember : PointerToMemberHelper<typename std::remove_cv<MP>::type> {};

template<typename MP>
class Property {
public:
    typedef typename PointerToMember<MP>::class_type class_type;
    typedef typename PointerToMember<MP>::variable_type variable_type;
    typedef typename PointerToMember<MP>::result_type T;

    Property(class_type* _this, MP member):
        this_(_this),
        getter_(member) {}

    Property() = delete;
    Property(const Property&) = delete;
    Property& operator=(const Property& rhs) {
        assert(this_); /* We don't copy this */
        getter_ = rhs.getter_;
    }

private:
    class_type* this_;
    property_impl::Getter<MP, class_type, variable_type> getter_;

public:

    inline operator T&() const {
        return *get();
    }

    inline T* operator->() const {
        return get();
    }

    inline operator T*() {
        return get();
    }

    T* get() const {
        return getter_.get(this_);
    }

    operator bool() const { return bool(get(this));}
};

}

#define S_DEFINE_PROPERTY(name, member) \
    Property<decltype(member)> name = {this, member}
