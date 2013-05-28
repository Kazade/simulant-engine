#ifndef MANAGER_H
#define MANAGER_H

#include "manager_base.h"

#include "kglt/kazbase/list_utils.h"

namespace kglt {
namespace generic {

template<typename Derived, typename ObjectType, typename ObjectIDType, typename NewIDGenerator=IncrementalGetNextID<ObjectIDType> >
class TemplatedManager : public virtual BaseManager {
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

    void manager_delete(ObjectIDType id) {
        if(manager_contains(id)) {
            std::lock_guard<std::recursive_mutex> lock(manager_lock_);

            ObjectType& obj = *objects_[id];
            signal_pre_delete_(obj, id);

            if(manager_contains(id)) {
                objects_.erase(id);
            }
        }
    }

    uint32_t manager_count() const {
        return objects_.size();
    }

    ObjectType& manager_get(ObjectIDType id) {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw NoSuchObjectError(typeid(ObjectType).name());
        }
        return *(it->second);
    }

    const ObjectType& manager_get(ObjectIDType id) const {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw NoSuchObjectError(typeid(ObjectType).name());
        }
        return *(it->second);
    }

    bool manager_contains(ObjectIDType id) const {
        return objects_.find(id) != objects_.end();
    }

    sigc::signal<void, ObjectType&, ObjectIDType>& signal_post_create() { return signal_post_create_; }
    sigc::signal<void, ObjectType&, ObjectIDType>& signal_pre_delete() { return signal_pre_delete_; }

    template<typename Func>
    void apply_func_to_objects(Func func) {
        for(std::pair<ObjectIDType, typename ObjectType::ptr> p: objects_) {
            std::bind(func, p.second.get())();
        }
    }

    //Internal!
    std::unordered_map<ObjectIDType, std::shared_ptr<ObjectType> > __objects() {
        return objects_;
    }
private:
    sigc::signal<void, ObjectType&, ObjectIDType> signal_post_create_;
    sigc::signal<void, ObjectType&, ObjectIDType> signal_pre_delete_;

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

}
}
#endif // MANAGER_H
