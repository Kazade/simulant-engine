#ifndef MANAGER_H
#define MANAGER_H

#include <type_traits>
#include "manager_base.h"

#include <functional>
#include "../deps/kazsignal/kazsignal.h"
#include "../deps/kazlog/kazlog.h"

namespace kglt {
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
            typename std::enable_if<std::is_pointer<typename Q::resource_pointer_type>::value>::type* = 0) {
        return get(*id).lock().get();
    }

    // This covers the case where the resource_pointer_type is a smart pointer
    template<typename Q=ObjectIDType>
    typename ObjectIDType::resource_pointer_type getter(
            const ObjectIDType* id,
            typename std::enable_if<!std::is_pointer<typename Q::resource_pointer_type>::value>::type* = 0) {
        return get(*id).lock();
    }

public:
    virtual ~TemplatedManager() {}

    typedef NewIDGenerator GeneratorType;
    typedef ObjectType Type;

    template<typename... Args>
    ObjectIDType make(Args&&... args) {
        return make(
            ObjectIDType(
                generator_(),
                [this](const ObjectIDType* id) -> typename ObjectIDType::resource_pointer_type {
                    return this->getter(id);
                }
            ),
            std::forward<Args>(args)...
        );
    }

    template<typename... Args>
    ObjectIDType make(ObjectIDType id, Args&&... args) {
        {
            std::lock_guard<std::recursive_mutex> lock(manager_lock_);

            objects_.insert(std::make_pair(id,
                ObjectType::create(id, std::forward<Args>(args)...)
            ));
        }

        signal_post_create_(*objects_[id], id);

        return id;
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

    void each(std::function<void (ObjectType*)> func) const {
        for(std::pair<ObjectIDType, typename ObjectType::ptr> p: objects_) {
            auto thing = get(p.first); //Make sure we lock the object
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
