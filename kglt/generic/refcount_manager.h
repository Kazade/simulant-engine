#ifndef REFCOUNT_MANAGER_H
#define REFCOUNT_MANAGER_H

#include "manager_base.h"
#include <kazbase/list_utils.h>
#include <kazsignal/kazsignal.h>
#include "protected_ptr.h"

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

    ObjectIDType manager_new(GarbageCollectMethod garbage_collect) {
        return manager_new(ObjectIDType(), garbage_collect);
    }

    template<typename ...Args>
    ObjectIDType manager_new(GarbageCollectMethod garbage_collect, Args&&... args) {
        return manager_new(ObjectIDType(), garbage_collect, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    ObjectIDType manager_new(ObjectIDType id, GarbageCollectMethod garbage_collect, Args&&... args) {
        std::lock_guard<std::mutex> lock(manager_lock_);
        if(!id) {
            id = NewIDGenerator()();
        }

        auto obj = ObjectType::create(id, std::forward<Args>(args)...);
        obj->enable_gc(garbage_collect);

        objects_.insert(std::make_pair(id, obj));
        creation_times_[id] = std::chrono::system_clock::now();
        uncollected_.insert(id);

        signal_post_create_(*objects_[id], id);

        return id;
    }

    uint32_t manager_count() const {
        return objects_.size();
    }

    std::weak_ptr<ObjectType> manager_unlocked_get(ObjectIDType id) const {
        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw DoesNotExist<ObjectType>(
                typeid(ObjectType).name() + _u("ID: {0}").format(
                    id.value()
                ).encode()
            );
        }

        uncollected_.erase(id);

        return std::weak_ptr<ObjectType>(it->second);
    }

    std::weak_ptr<ObjectType> manager_get(ObjectIDType id) {
        std::lock_guard<std::mutex> lock(manager_lock_);
        return manager_unlocked_get(id);
    }

    ObjectType* manager_get_unsafe(ObjectIDType id) {
        std::lock_guard<std::mutex> lock(manager_lock_);

        return manager_unlocked_get(id).lock().get();
    }

    const std::weak_ptr<ObjectType> manager_get(ObjectIDType id) const {
        std::lock_guard<std::mutex> lock(manager_lock_);
        return manager_unlocked_get(id);
    }

    bool manager_contains(ObjectIDType id) const {
        return objects_.find(id) != objects_.end();
    }

    sig::signal<void (ObjectType&, ObjectIDType)>& signal_post_create() { return signal_post_create_; }
    sig::signal<void (ObjectType&, ObjectIDType)>& signal_pre_delete() { return signal_pre_delete_; }

    //Internal!
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > __objects() {
        return objects_;
    }

    void each(std::function<void (ObjectType*)> func) const {
        std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > objects;
        {
            // We copy the object IDs so we don't keep a handle to
            // any shared_ptrs or anything
            std::lock_guard<std::mutex> lock(manager_lock_);
            objects = objects_;
        }

        for(auto& p: objects) {
            try {
                auto thing = manager_get(p.first).lock();
                func(thing.get());
            } catch(DoesNotExist<ObjectType>& e) {
                // May have been deleted in another thread, just ignore this
                continue;
            }
        }
    }

    void garbage_collect() {
        std::lock_guard<std::mutex> lock(manager_lock_);

        for(ObjectIDType key: container::keys(objects_)) {
            if(objects_[key].unique() && objects_[key]->uses_gc()) {
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

                    ok_to_delete = lifetime_in_seconds > 10;

                    if(ok_to_delete) {
                        L_WARN("Deleting unclaimed resource");
                    }
                }

                if(ok_to_delete) {
                    objects_.erase(key);
                    creation_times_.erase(key);

                    L_DEBUG(_u("Garbage collected: {0}").format(key.value()));
                }
            }
        }
    }

    typedef std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType>> ObjectMap;

protected:
    void manager_store_alias(const unicode& alias, ObjectIDType id) {
        auto it = object_names_.find(alias);
        if(it != object_names_.end()) {
            if((*it).second == id) {
                //Don't throw if it's the same thing
                return;
            }
            throw LogicError(_u("Attempted to add a duplicate resource with the alias: {0}").format(alias).encode());
        }

        object_names_[alias] = id;
    }

    ObjectIDType manager_get_by_alias(const unicode& alias) {
        auto it = object_names_.find(alias);
        if(it == object_names_.end()) {
            throw DoesNotExist<ObjectIDType>(_u("Object with alias {0} does not exist").format(alias));
        }

        return (*it).second;
    }

    void manage_remove_alias(const unicode& alias) {
        object_names_.erase(alias);
    }

private:
    typedef std::chrono::time_point<std::chrono::system_clock> date_time;

    ObjectMap objects_;
    std::unordered_map<ObjectIDType, date_time> creation_times_;
    mutable std::set<ObjectIDType> uncollected_;

    sig::signal<void (ObjectType&, ObjectIDType)> signal_post_create_;
    sig::signal<void (ObjectType&, ObjectIDType)> signal_pre_delete_;

    std::unordered_map<unicode, ObjectIDType> object_names_;

};

}
}

#endif // REFCOUNT_MANAGER_H
