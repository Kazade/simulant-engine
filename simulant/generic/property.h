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

#ifndef PROPERTY_H
#define PROPERTY_H

#include <functional>
#include <memory>

template<typename Container, typename T>
class Property {
public:
    Property(Container* _this, T Container::* member):
        this_(_this),
        getter_([member](Container* self) -> T* {
            return &(self->*member);
        }) {
    }

    Property(Container* _this, T* Container::* member):
        this_(_this),
        getter_([member](Container* self) -> T* {
            return (self->*member);
        }) {
    }

    Property(Container* _this, std::unique_ptr<T> Container::* member):
        this_(_this),
        getter_([member](Container* self) -> T* {
            return (self->*member).get();
    }) {

    }

    Property(Container* _this, std::shared_ptr<T> Container::* member):
        this_(_this),
        getter_([member](Container* self) -> T* {
            return (self->*member).get();
        }) {

    }

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
private:
    Container* this_ = nullptr;
    std::function<T* (Container*)> getter_;

};



#endif // PROPERTY_H

