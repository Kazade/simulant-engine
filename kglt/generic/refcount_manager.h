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
class RefCountedTemplatedManager :
    public virtual BaseManager {
public:
    ObjectIDType manager_new() {
        ObjectIDType id(0);
        {
            std::lock_guard<std::recursive_mutex> lock(manager_lock_);
            id = NewIDGenerator()();
            objects_.insert(std::make_pair(id, typename ObjectType::ptr(new ObjectType((Derived*)this, id))));
        }

        creation_times_[id] = std::chrono::system_clock::now();

        signal_post_create_(*objects_[id], id);

        return id;
    }

    uint32_t manager_count() const {
        return objects_.size();
    }

    std::weak_ptr<ObjectType> manager_get(ObjectIDType id) {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw NoSuchObjectError(typeid(ObjectType).name());
        }

        return std::weak_ptr<ObjectType>(it->second);
    }

    ObjectType* manager_get_unsafe(ObjectIDType id) {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw NoSuchObjectError(typeid(ObjectType).name());
        }

        return it->second.get();
    }

    const std::weak_ptr<ObjectType> manager_get(ObjectIDType id) const {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw NoSuchObjectError(typeid(ObjectType).name());
        }

        return std::weak_ptr<ObjectType>(it->second);
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
        for(ObjectIDType key: container::keys(objects_)) {
            if(objects_[key].unique()) {
                date_time now = std::chrono::system_clock::now();

                /* Little hacky maybe? We give objects a grace period of
                 * 1 second to store a reference. This should be plenty of time!
                 *
                 * This feels really dirty, but the only alternative is to return
                 * a ProtectedPtr<T> or shared_ptr<T> from new_X() which is even more horrible..
                 *
                 */

                int lifetime_in_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                    now-creation_times_[key]
                ).count();

                if(lifetime_in_seconds > 1) {
                    objects_.erase(key);
                    creation_times_.erase(key);

                    L_DEBUG(_u("Garbage collected: {0}").format(key.value()));
                }
            }
        }
    }

private:
    typedef std::chrono::time_point<std::chrono::system_clock> date_time;

    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > objects_;
    std::unordered_map<ObjectIDType, date_time> creation_times_;

    sigc::signal<void, ObjectType&, ObjectIDType> signal_post_create_;
    sigc::signal<void, ObjectType&, ObjectIDType> signal_pre_delete_;

};

}
}

#endif // REFCOUNT_MANAGER_H
