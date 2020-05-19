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

#include <cstdint>
#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <ostream>
#include <typeindex>

namespace smlt {

namespace _unique_id_impl {
    template<class T>
    struct is_shared_ptr : std::false_type {};

    template<class T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

    template<typename ResourceTypePtr, bool IsSharedPtr>
    struct Members;

    template<typename ResourceTypePtr>
    struct Members<ResourceTypePtr, false> {
        ResourceTypePtr ptr;

        Members():
            ptr(nullptr) {}

        Members(ResourceTypePtr ptr):
            ptr(ptr) {}

        ResourceTypePtr as_pointer() const {
            return ptr;
        }
    };

    template<typename ResourceTypePtr>
    struct Members<ResourceTypePtr, true> {
        typedef std::weak_ptr<typename ResourceTypePtr::element_type> weak_ptr_type;
        weak_ptr_type ptr;

        Members() = default;

        Members(weak_ptr_type ptr):
            ptr(ptr) {}

        Members(ResourceTypePtr ptr):
            ptr(ptr) {}

        ResourceTypePtr as_pointer() const {
            return ptr.lock();
        }
    };
}


template<typename ResourceTypePtr>
class UniqueID {
public:
    typedef ResourceTypePtr resource_pointer_type;
    typedef typename ResourceTypePtr::element_type element_type;

    explicit operator bool() const {
        return id_ > 0;
    }

    UniqueID():
        id_(0) {}

    /* Unbound. This should only happen during instantiation by a manager */
    UniqueID(uint32_t id):
        id_(id) {
    }

    UniqueID(const UniqueID<ResourceTypePtr>& other):
        id_(other.id_),
        members_(other.members_.ptr) {

    }

    /* Private API */
    void _bind(resource_pointer_type ptr) {
        members_ = MembersType(ptr);
    }

    ResourceTypePtr fetch() const {
        auto ret = members_.as_pointer();
        return ret;
    }

    template<typename T>
    T* fetch_as() const {
        /* The weird &(* casting is to handle smart pointers */
        return dynamic_cast<T*>(&(*fetch()));
    }

    bool is_bound() const {
        return bool(members_.ptr);
    }

    explicit UniqueID(uint32_t id, ResourceTypePtr ptr):
        id_(id),
        members_(ptr) {

    }

    bool operator==(const UniqueID<ResourceTypePtr>& other) const {
        return this->id_ == other.id_;
    }

    /* Implicit conversion from shared ptr type */
    template<typename=std::enable_if<_unique_id_impl::is_shared_ptr<resource_pointer_type>::value>>
    UniqueID(const resource_pointer_type& pointer) {
        this->id_ = pointer->id().id_;
        _bind(pointer);
    }

    template<typename=std::enable_if<_unique_id_impl::is_shared_ptr<resource_pointer_type>::value>>
    UniqueID& operator=(const resource_pointer_type& pointer) {
        this->id_ = pointer->id().id_;
        _bind(pointer);

        return *this;
    }
    /* Conversion ends */

    UniqueID& operator=(const UniqueID<ResourceTypePtr>& other) {
        if(&other == this) {
            return *this;
        }

        this->id_ = other.id_;
        this->members_.ptr = other.members_.ptr;
        return *this;
    }

    bool operator<(const UniqueID<ResourceTypePtr>& other) const {
        return this->id_ < other.id_;
    }

    bool operator!=(const UniqueID<ResourceTypePtr>& other) const {
        return !(*this == other);
    }

    friend std::ostream& operator<< (std::ostream& o, UniqueID<ResourceTypePtr> const& instance) {
        return o << typeid(ResourceTypePtr).name() << ":" << instance.value();
    }

    uint32_t value() const { return id_; }

private:
    uint32_t id_ = 0;

    typedef _unique_id_impl::Members<
        ResourceTypePtr,
        _unique_id_impl::is_shared_ptr<ResourceTypePtr>::value
    > MembersType;

    MembersType members_;
};

typedef std::pair<std::type_index, uint32_t> UniqueIDKey;

template<typename ID>
UniqueIDKey make_unique_id_key(const ID& id) {
    return std::make_pair(
        std::type_index(typeid(typename ID::element_type)),
        id.value()
    );
}

template<typename UniqueID>
UniqueID make_unique_id_from_key(const UniqueIDKey& key) {
    assert(key.first == std::type_index(typeid(typename UniqueID::element_type)));
    return UniqueID(key.second);
}


}

namespace std {
    template<typename ResourcePtrType>
    struct hash< smlt::UniqueID<ResourcePtrType> > {
        size_t operator()(const smlt::UniqueID<ResourcePtrType>& id) const {
            hash<uint32_t> make_hash;
            return make_hash(id.value());
        }
    };
}


