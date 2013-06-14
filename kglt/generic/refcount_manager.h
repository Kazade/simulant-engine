#ifndef REFCOUNT_MANAGER_H
#define REFCOUNT_MANAGER_H

#include "manager_base.h"
#include "../kazbase/list_utils.h"

namespace kglt {
namespace generic {

template<
    typename Derived,
    typename ObjectType,
    typename ObjectIDType,
    typename NewIDGenerator=IncrementalGetNextID<ObjectIDType>
>
class RefCountedTemplatedManager {
protected:
    mutable std::mutex manager_lock_;

public:
    ObjectIDType manager_new() {
        ObjectIDType id(0);
        {
            std::lock_guard<std::mutex> lock(manager_lock_);
            id = NewIDGenerator()();
            objects_.insert(std::make_pair(id, typename ObjectType::ptr(new ObjectType((Derived*)this, id))));
            creation_times_[id] = std::chrono::system_clock::now();
            uncollected_.insert(id);
        }

        signal_post_create_(*objects_[id], id);

        return id;
    }

    ObjectIDType manager_clone(ObjectIDType orig) {
        ObjectIDType id(0);
        {
            std::lock_guard<std::mutex> lock(manager_lock_);
            id = NewIDGenerator()();

            typename ObjectType::ptr new_obj(new ObjectType((Derived*)this, id));
            objects_.insert(std::make_pair(id, new_obj));

            creation_times_[id] = std::chrono::system_clock::now();
            uncollected_.insert(id);

            //Copy the original object
            *new_obj = *manager_unlocked_get(orig).lock();
        }

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

    sigc::signal<void, ObjectType&, ObjectIDType>& signal_post_create() { return signal_post_create_; }
    sigc::signal<void, ObjectType&, ObjectIDType>& signal_pre_delete() { return signal_pre_delete_; }

    //Internal!
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > __objects() {
        return objects_;
    }

    void garbage_collect() {
        std::lock_guard<std::mutex> lock(manager_lock_);

        for(ObjectIDType key: container::keys(objects_)) {
            if(objects_[key].unique()) {                
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

private:
    typedef std::chrono::time_point<std::chrono::system_clock> date_time;

    ObjectMap objects_;
    std::unordered_map<ObjectIDType, date_time> creation_times_;
    mutable std::set<ObjectIDType> uncollected_;

    sigc::signal<void, ObjectType&, ObjectIDType> signal_post_create_;
    sigc::signal<void, ObjectType&, ObjectIDType> signal_pre_delete_;

};

}
}

#endif // REFCOUNT_MANAGER_H
