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

#ifndef UNIQUE_ID_H
#define UNIQUE_ID_H

#include <cstdint>
#include <functional>


template<typename ResourceTypePtr>
class UniqueID {
public:
    typedef ResourceTypePtr resource_pointer_type;

    typedef std::function<ResourceTypePtr (const UniqueID<ResourceTypePtr>*)> ResourceGetter;

    operator bool() const {
        return id_ > 0;
    }

    UniqueID(const UniqueID<ResourceTypePtr>& other):
        id_(other.id_),
        getter_(other.getter_) {

    }

    UniqueID(ResourceGetter getter=ResourceGetter()):
        id_(0),
        getter_(getter) {}

    ResourceTypePtr fetch() const {
        assert(is_bound() && "This ID is not bound to a resource manager");
        return getter_(this);
    }

    template<typename T>
    T* fetch_as() const {
        /* The weird &(* casting is to handle smart pointers */
        return dynamic_cast<T*>(&(*fetch()));
    }

    bool is_bound() const {
        return bool(getter_);
    }

    explicit UniqueID(uint32_t id, ResourceGetter getter=ResourceGetter()):
        id_(id),
        getter_(getter) {

    }

    bool operator==(const UniqueID<ResourceTypePtr>& other) const {
        return this->id_ == other.id_;
    }

    UniqueID& operator=(const UniqueID<ResourceTypePtr>& other) {
        if(&other == this) {
            return *this;
        }

        this->id_ = other.id_;
        this->getter_ = other.getter_;
        return *this;
    }

    bool operator<(const UniqueID<ResourceTypePtr>& other) const {
        return this->id_ < other.id_;
    }

    bool operator!=(const UniqueID<ResourceTypePtr>& other) const {
        return !(*this == other);
    }

    friend std::ostream& operator<< (std::ostream& o, UniqueID<ResourceTypePtr> const& instance) {
        return o << instance.value();
    }

    uint32_t value() const { return id_; }

private:
    uint32_t id_ = 0;
    ResourceGetter getter_;
};

namespace std {
    template<typename ResourcePtrType>
    struct hash< UniqueID<ResourcePtrType> > {
        size_t operator()(const UniqueID<ResourcePtrType>& id) const {
            hash<uint32_t> make_hash;
            return make_hash(id.value());
        }
    };
}


#endif // UNIQUE_ID_H
