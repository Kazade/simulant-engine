#ifndef REFCOUNT_MANAGER_H
#define REFCOUNT_MANAGER_H

#include <set>
#include "../deps/kazsignal/kazsignal.h"
#include "../deps/kazlog/kazlog.h"

#include "manager_base.h"


namespace kglt {

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

public:
    void mark_as_uncollected(ObjectIDType id) {
        std::lock_guard<std::mutex> lock(manager_lock_);
        uncollected_.insert(id);
    }

    ObjectIDType make(GarbageCollectMethod garbage_collect) {
        return make(ObjectIDType(), garbage_collect);
    }

    template<typename ...Args>
    ObjectIDType make(GarbageCollectMethod garbage_collect, Args&&... args) {
        return make(ObjectIDType(), garbage_collect, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    ObjectIDType make(ObjectIDType id, GarbageCollectMethod garbage_collect, Args&&... args) {
        std::lock_guard<std::mutex> lock(manager_lock_);
        if(!id) {
            id = ObjectIDType(
                NewIDGenerator()(),
                [this](const ObjectIDType* id) -> typename ObjectIDType::resource_pointer_type {
                    return this->get(*id).lock();
                }
            );
        }

        auto obj = ObjectType::create(id, std::forward<Args>(args)...);
        assert(obj);

        obj->enable_gc(garbage_collect == GARBAGE_COLLECT_PERIODIC);

        assert(id);

        objects_.insert(std::make_pair(id, obj));
        creation_times_[id] = std::chrono::system_clock::now();
        uncollected_.insert(id);

        signal_post_create_(*obj, id);

        return id;
    }

    uint32_t count() const {
        return objects_.size();
    }

    std::weak_ptr<ObjectType> manager_unlocked_get(ObjectIDType id) const {
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
        return objects_.find(id) != objects_.end();
    }

    sig::signal<void (ObjectType&, ObjectIDType)>& signal_post_create() { return signal_post_create_; }
    sig::signal<void (ObjectType&, ObjectIDType)>& signal_pre_delete() { return signal_pre_delete_; }

    //Internal!
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > __objects() {
        return objects_;
    }

    void each(std::function<void (ObjectType*)> func) const {
        std::set<ObjectIDType> object_ids;
        {
            // We copy the object IDs so we don't keep a handle to
            // any shared_ptrs or anything
            std::lock_guard<std::mutex> lock(manager_lock_);
            for(auto& p: objects_) {
                object_ids.insert(p.first);
            }
        }

        for(auto& obj_id: object_ids) {
            auto thing = get(obj_id).lock();

            // If thing is false-y it may have been deleted in another thread
            if(thing) {
                func(thing.get());
            }
        }
    }

    void garbage_collect() {
        std::lock_guard<std::mutex> lock(manager_lock_);

        for(auto obj_it = objects_.begin(); obj_it != objects_.end(); ) {
            auto& key = obj_it->first;
            assert(key);

            auto& pointer = obj_it->second;
            assert(pointer);

            if(pointer.unique() && pointer->uses_gc()) {
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
                        now-creation_times_[key]
                    ).count();

                    ok_to_delete = lifetime_in_seconds > 20;

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

protected:
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
