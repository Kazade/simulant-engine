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

#ifndef MANAGER_H
#define MANAGER_H

#include <type_traits>
#include "manager_base.h"

#include <functional>
#include "../deps/kazsignal/kazsignal.h"
#include "../deps/kazlog/kazlog.h"

namespace smlt {
namespace generic {


template<typename ObjectType, typename ObjectIDType, typename NewIDGenerator=IncrementalGetNextID<ObjectIDType> >
class TemplatedManager {
protected:
    mutable std::recursive_mutex manager_lock_;


private:
    // This covers the case where the resource_pointer_type is a raw pointer
    template<typename Q=ObjectIDType>
    typename ObjectIDType::resource_pointer_type getter(
            const ObjectIDType* id,
            typename std::enable_if<
                std::is_convertible<typename Q::resource_pointer_type, ObjectType*>::value
            >::type* = 0) {
        return get(*id).lock().get();
    }

    // This covers the case where the resource_pointer_type is a smart pointer
    template<typename Q=ObjectIDType>
    typename ObjectIDType::resource_pointer_type getter(
            const ObjectIDType* id,
            typename std::enable_if<
                !std::is_convertible<typename Q::resource_pointer_type, ObjectType*>::value
            >::type* = 0) {
        return get(*id).lock();
    }

public:
    virtual ~TemplatedManager() {}

    typedef NewIDGenerator GeneratorType;
    typedef ObjectType Type;

    template<typename... Args>
    ObjectIDType make(Args&&... args) {
        return make_as<ObjectType>(std::forward<Args>(args)...);
    }

    template<typename... Args>
    ObjectIDType make(ObjectIDType id, Args&&... args) {
        return make_as<ObjectType>(id, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    ObjectIDType make_as(ObjectIDType id, Args&&... args) {
        {

            // Make the new object, but dont lock until we insert
            // this prevents deadlocks being caused if a stage is created in another
            // thread and needs the idle task handler to run
            auto to_insert = std::make_pair(id,
                T::create(id, std::forward<Args>(args)...)
            );

            std::lock_guard<std::recursive_mutex> lock(manager_lock_);
            objects_.insert(to_insert);
        }

        signal_post_create_(*objects_[id], id);

        return id;
    }

    template<typename T, typename... Args>
    ObjectIDType make_as(Args&&... args) {
        return make_as<T>(
            ObjectIDType(
                generator_(),
                [this](const ObjectIDType* id) -> typename ObjectIDType::resource_pointer_type {
                    return this->getter(id);
                }
            ),
            std::forward<Args>(args)...
        );
    }

    void destroy_all() {
        for(auto p: objects_) {
            signal_pre_delete_(*p.second, p.first);
        }

        objects_.clear();
    }

    void destroy(ObjectIDType id) {
        if(contains(id)) {
            std::lock_guard<std::recursive_mutex> lock(manager_lock_);

            ObjectType& obj = *objects_[id];
            signal_pre_delete_(obj, id);

            if(contains(id)) {
                objects_.erase(id);
            }
        }
    }

    uint32_t count() const {
        return objects_.size();
    }

    std::weak_ptr<ObjectType> get(ObjectIDType id) {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            L_WARN(_F(
                "Unable to find object of type: {0} with ID {1}").format(
                    typeid(ObjectType).name(),
                    id
                )
            );
            return std::weak_ptr<ObjectType>();
        }
        return it->second;
    }

    /**
     * @brief first
     * @return Returns an unspecified item from the manager. This is useful in tests.
     * It's not neccessarily random but effectively it is as far as you care.
     */
    std::weak_ptr<ObjectType> first() const {
        return objects_.begin()->second;
    }

    /**
     * @brief only
     * @return Returns the only item in the container, or throws a LogicError
     * if there is more than one item or DoesNotExist<ObjectType> if the manager is empty
     */
    std::weak_ptr<ObjectType> only() const {
        if(count() != 1) {
            if(count() == 0) {
                throw ObjectLookupError("Container does not contain any objects");
            }
            throw ObjectLookupError("Used only() on manager with more than one item");
        }
        return first();
    }

    std::weak_ptr<ObjectType> get(ObjectIDType id) const {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            L_WARN(_F(
                "Unable to find object of type: {0} with ID {1}").format(
                    typeid(ObjectType).name(),
                    id
                )
            );
            return std::weak_ptr<ObjectType>();
        }
        return it->second;
    }

    bool contains(ObjectIDType id) const {
        return objects_.find(id) != objects_.end();
    }

    sig::signal<void (ObjectType&, ObjectIDType)>& signal_post_create() { return signal_post_create_; }
    sig::signal<void (ObjectType&, ObjectIDType)>& signal_pre_delete() { return signal_pre_delete_; }

    void each(std::function<void (uint32_t, ObjectType*)> func) const {
        uint32_t i = 0;

        for(std::pair<ObjectIDType, typename ObjectType::ptr> p: objects_) {
            auto thing = get(p.first); //Make sure we lock the object
            auto ptr = thing.lock(); // Keep the shared_ptr around while we use it
            func(i++, ptr.get());
        }
    }

    //Internal!
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> >& __objects() {
        return objects_;
    }

private:
    sig::signal<void (ObjectType&, ObjectIDType)> signal_post_create_;
    sig::signal<void (ObjectType&, ObjectIDType)> signal_pre_delete_;

    static NewIDGenerator generator_;

protected:
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > objects_;

    ObjectIDType _get_object_id_from_ptr(ObjectType* ptr) {
        for(std::pair<ObjectIDType, std::shared_ptr<ObjectType> > pair: objects_) {
            if(pair.second.get() == ptr) {
                return pair.first;
            }
        }

        return ObjectIDType();
    }
};

template <typename ObjectType, typename ObjectIDType, typename NewIDGenerator>
NewIDGenerator TemplatedManager<ObjectType, ObjectIDType, NewIDGenerator>::generator_;


}
}
#endif // MANAGER_H
