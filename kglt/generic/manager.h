#ifndef MANAGER_H
#define MANAGER_H

#include "manager_base.h"

#include <functional>
#include <kazbase/list_utils.h>
#include "kazsignal/kazsignal.h"

namespace kglt {
namespace generic {

template<typename ObjectType, typename ObjectIDType, typename NewIDGenerator=IncrementalGetNextID<ObjectIDType> >
class TemplatedManager {
protected:
    mutable std::recursive_mutex manager_lock_;

private:

public:
    virtual ~TemplatedManager() {}

    typedef NewIDGenerator GeneratorType;
    typedef ObjectType Type;

    template<typename... Args>
    ObjectIDType manager_new(Args&&... args) {
        return manager_new(generator_(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    ObjectIDType manager_new(ObjectIDType id, Args&&... args) {
        {
            std::lock_guard<std::recursive_mutex> lock(manager_lock_);

            objects_.insert(std::make_pair(id,
                ObjectType::create(id, std::forward<Args>(args)...)
            ));
        }

        signal_post_create_(*objects_[id], id);

        return id;
    }

    void manager_delete_all() {
        for(auto p: objects_) {
            signal_pre_delete_(*p.second, p.first);
        }

        objects_.clear();
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

    std::weak_ptr<ObjectType> manager_get(ObjectIDType id) {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw DoesNotExist<ObjectType>(typeid(ObjectType).name() + _u(" with ID: {0}").format(id));
        }
        return it->second;
    }

    /**
     * @brief manager_any
     * @return Returns an unspecified item from the manager. This is useful in tests.
     * It's not neccessarily random but effectively it is as far as you care.
     */
    std::weak_ptr<ObjectType> manager_any() const {
        return objects_.begin()->second;
    }

    /**
     * @brief manager_only
     * @return Returns the only item in the container, or throws a LogicError
     * if there is more than one item or DoesNotExist<ObjectType> if the manager is empty
     */
    std::weak_ptr<ObjectType> manager_only() const {
        if(manager_count() != 1) {
            if(manager_count() == 0) {
                throw DoesNotExist<ObjectType>("Container does not contain any objects");
            }
            throw LogicError("Used only() on manager with more than one item");
        }
        return manager_any();
    }

    std::weak_ptr<ObjectType> manager_get(ObjectIDType id) const {
        std::lock_guard<std::recursive_mutex> lock(manager_lock_);

        auto it = objects_.find(id);
        if(it == objects_.end()) {
            throw DoesNotExist<ObjectType>(typeid(ObjectType).name() + _u(" with ID: {0}").format(id));
        }
        return it->second;
    }

    bool manager_contains(ObjectIDType id) const {
        return objects_.find(id) != objects_.end();
    }

    sig::signal<void (ObjectType&, ObjectIDType)>& signal_post_create() { return signal_post_create_; }
    sig::signal<void (ObjectType&, ObjectIDType)>& signal_pre_delete() { return signal_pre_delete_; }

    void each(std::function<void (ObjectType*)> func) const {
        for(std::pair<ObjectIDType, typename ObjectType::ptr> p: objects_) {
            auto thing = manager_get(p.first); //Make sure we lock the object
            auto ptr = thing.lock(); // Keep the shared_ptr around while we use it
            func(ptr.get());
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
