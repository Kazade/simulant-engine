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
                objects_.erase(key);
                L_DEBUG(_u("Garbage collected: {0}").format(key.value()));
            }
        }
    }

private:
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > objects_;

    sigc::signal<void, ObjectType&, ObjectIDType> signal_post_create_;
    sigc::signal<void, ObjectType&, ObjectIDType> signal_pre_delete_;

};

}
}

#endif // REFCOUNT_MANAGER_H
