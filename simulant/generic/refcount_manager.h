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

#ifndef REFCOUNT_MANAGER_H
#define REFCOUNT_MANAGER_H

#include <set>
#include <list>
#include "../deps/kazsignal/kazsignal.h"
#include "../deps/kazlog/kazlog.h"

#include "manager_base.h"


namespace smlt {

enum GarbageCollectMethod {
    GARBAGE_COLLECT_NEVER,
    GARBAGE_COLLECT_PERIODIC
};

namespace generic {

template<
    typename ObjectType,
    typename ObjectIDType,
    typename NewIDGenerator=IncrementalGetNextID<ObjectIDType>
>
class RefCountedTemplatedManager {
protected:
    mutable std::mutex manager_lock_;


private:
    ObjectIDType generate_new_id() {
        return ObjectIDType(
            NewIDGenerator()(),
            [this](const ObjectIDType* id) -> typename ObjectIDType::resource_pointer_type {
                return this->get(*id).lock();
            }
        );
    }

public:
    void mark_as_uncollected(ObjectIDType id) {
        std::lock_guard<std::mutex> lock(manager_lock_);
        uncollected_.insert(id);
    }

    ObjectIDType make(GarbageCollectMethod garbage_collect) {
        return make(generate_new_id(), garbage_collect);
    }

    template<typename ...Args>
    ObjectIDType make(GarbageCollectMethod garbage_collect, Args&&... args) {
        return make(generate_new_id(), garbage_collect, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    ObjectIDType make(ObjectIDType id, GarbageCollectMethod garbage_collect, Args&&... args) {

        if(!id) {
            std::lock_guard<std::mutex> lock(manager_lock_);
            id = generate_new_id();
            assert(id);
        }

        /* We intentionally create the object outside of the resource manager
         * lock, otherwise we can end up with deadlocks if this is happening
         * in a thread other than the main thread, and we need something to
         * run on idle.
         */
        auto obj = ObjectType::create(id, std::forward<Args>(args)...);
        assert(obj);

        /*
         * FIXME: Should be more options than on or off
         */
        obj->enable_gc(garbage_collect == GARBAGE_COLLECT_PERIODIC);

        {
            /* Update the containers within a lock */
            std::lock_guard<std::mutex> lock(manager_lock_);
            objects_.insert(std::make_pair(id, obj));
            creation_times_.insert(std::make_pair(id, std::chrono::system_clock::now()));
            uncollected_.insert(id);
        }

        signal_post_create_(*obj, id);

        return id;
    }

    std::size_t count() const {
        return objects_.size();
    }

    std::weak_ptr<ObjectType> manager_unlocked_get(ObjectIDType id) const {
        assert(id);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            L_WARN(_F("Unable to locate object of type {0} with ID {1}").format(typeid(ObjectType).name(), id));
            return std::weak_ptr<ObjectType>();
        }

        uncollected_.erase(id);

        return std::weak_ptr<ObjectType>(it->second);
    }

    std::weak_ptr<ObjectType> get(ObjectIDType id) {
        std::lock_guard<std::mutex> lock(manager_lock_);
        return manager_unlocked_get(id);
    }

    ObjectType* get_unsafe(ObjectIDType id) {
        std::lock_guard<std::mutex> lock(manager_lock_);

        return manager_unlocked_get(id).lock().get();
    }

    const std::weak_ptr<ObjectType> get(ObjectIDType id) const {
        std::lock_guard<std::mutex> lock(manager_lock_);
        return manager_unlocked_get(id);
    }

    bool contains(ObjectIDType id) const {
        return objects_.count(id) > 0;
    }

    sig::signal<void (ObjectType&, ObjectIDType)>& signal_post_create() { return signal_post_create_; }
    sig::signal<void (ObjectType&, ObjectIDType)>& signal_pre_delete() { return signal_pre_delete_; }

    //Internal!
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > __objects() {
        return objects_;
    }

    void each(std::function<void (ObjectType*)> func) const {
        typedef std::shared_ptr<ObjectType> ObjectTypePtr;
        std::list<ObjectTypePtr> objects;

        {
            // Create a list of shared pointers so that they don't go out of scope
            std::lock_guard<std::mutex> lock(manager_lock_);
            for(auto& p: objects_) {
                objects.push_back(p.second);
            }
        }

        for(ObjectTypePtr& ptr: objects) {
            ObjectType& thing = *ptr;
            if(thing.uses_gc() && (ptr.use_count() <= 2)) {
                // If the object is garbaged collected and either this it the
                // only shared_ptr, or there are two (e.g. one here, one in objects_)
                // then ignore
                continue;
            }

            func(&thing);
        }
    }

    void garbage_collect() {
        std::lock_guard<std::mutex> lock(manager_lock_);

        for(auto obj_it = objects_.begin(); obj_it != objects_.end(); ) {
            auto key = obj_it->first;
            assert(key);


            bool pointer_is_unique = obj_it->second.unique();
            ObjectType* obj = obj_it->second.get();

            assert(obj);

            if(pointer_is_unique && obj->uses_gc()) {
                auto it = uncollected_.find(key);
                bool ok_to_delete = false;

                if(it == uncollected_.end()) {
                    //If the object has been accessed, then we can assume
                    //that it's been used and no longer needed
                    ok_to_delete = true;
                } else {
                    //Otherwise, if the object hasn't been accessed after 10 seconds
                    //of being alive then delete it.
                    date_time now = std::chrono::system_clock::now();

                    int lifetime_in_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                        now - creation_times_.at(key)
                    ).count();

                    ok_to_delete = lifetime_in_seconds > 5;

                    if(ok_to_delete) {
                        L_WARN("Deleting unclaimed resource");
                    }
                }

                if(ok_to_delete) {
                    obj_it = objects_.erase(obj_it);
                    creation_times_.erase(key);
                    L_DEBUG(_F("Garbage collected: {0}").format(key.value()));
                    continue; // Don't increment the iterator
                }
            }

            ++obj_it;
        }
    }

    typedef std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType>> ObjectMap;


    void manager_store_alias(const std::string& alias, ObjectIDType id) {
        auto it = object_names_.find(alias);
        if(it != object_names_.end()) {
            if((*it).second == id) {
                //Don't throw if it's the same thing
                return;
            } else {
                L_DEBUG(_F("Overwriting object with alias: {0}").format(alias));
            }
        }

        object_names_[alias] = id;
    }

    ObjectIDType get_by_alias(const std::string& alias) {
        auto it = object_names_.find(alias);
        if(it == object_names_.end()) {
            L_WARN(_F("Unable to find ID for alias: {0}").format(alias));
            return ObjectIDType();
        }

        return (*it).second;
    }

    void manage_remove_alias(const std::string& alias) {
        object_names_.erase(alias);
    }

private:
    typedef std::chrono::time_point<std::chrono::system_clock> date_time;

    ObjectMap objects_;
    std::unordered_map<ObjectIDType, date_time> creation_times_;
    mutable std::set<ObjectIDType> uncollected_;

    sig::signal<void (ObjectType&, ObjectIDType)> signal_post_create_;
    sig::signal<void (ObjectType&, ObjectIDType)> signal_pre_delete_;

    std::unordered_map<std::string, ObjectIDType> object_names_;

};

}
}

#endif // REFCOUNT_MANAGER_H
