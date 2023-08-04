#pragma once

#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "default_init_ptr.h"
#include "../core/stage_node_id.h"

#include "../logging.h"
#include "../signals/signal.h"
#include "../macros.h"

namespace smlt {

const bool DONT_REFCOUNT = false;
const bool DO_REFCOUNT = true;

namespace _object_manager_impl {

/* All managers of the same type should share a counter */
template<typename T>
class IDCounter {
public:
    static uint32_t next_id() {
        static uint32_t id = 0;
        return ++id;
    }
};

template<typename IDType, typename ObjectType, typename ObjectTypePtrType, typename SmartPointerConverter>
class ObjectManagerBase {
public:
    typedef ObjectManagerBase<IDType, ObjectType, ObjectTypePtrType, SmartPointerConverter> this_type;
    typedef ObjectTypePtrType ObjectTypePtr;
    typedef typename ObjectTypePtrType::element_type object_type;

    virtual ~ObjectManagerBase() {
    }

    virtual void update() = 0;

    uint32_t count() const {
        return objects_.size();
    }

    /* Clones an object and returns an new ID to the clone */
    ObjectTypePtrType clone(IDType id, this_type* target_manager=nullptr) {
        if(!target_manager) {
            target_manager = this;
        }

        auto source = get(id);
        auto copy = target_manager->make(&source->asset_manager());
        *copy = *source;

        return copy;
    }

    template<typename... Args>
    ObjectTypePtrType make(Args&&... args) {
        return make_as<ObjectType>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    ObjectTypePtrType make_as(Args&&... args) {
        IDType new_id(next_id()); // Unbound

        S_DEBUG("Creating a new object with ID: {0}", new_id);
        auto obj = T::create(new_id, std::forward<Args>(args)...);
        objects_.insert(std::make_pair(obj->id(), obj));
        on_make(obj->id());

        return SmartPointerConverter::convert(obj);
    }

    void destroy(IDType id) {
        on_destroy(id);
        objects_.erase(id);
    }

    void destroy_all() {
        for(auto& p: objects_) {
            on_destroy(p.first);
        }

        objects_.clear();
    }

    ObjectTypePtr get(IDType id) const {
        auto it = objects_.find(id);
        if(it == objects_.end()) {
            return ObjectTypePtr();
        }

        return SmartPointerConverter::convert(it->second);
    }

    bool contains(IDType id) const {
        return objects_.count(id) > 0;
    }

    void each(std::function<void (uint32_t, ObjectTypePtr)> callback) {
        uint32_t i = 0;
        for(auto& p: objects_) {
            auto ptr = p.second;
            callback(i++, SmartPointerConverter::convert(ptr));
        }
    }

    void each(std::function<void (uint32_t, const ObjectTypePtr)> callback) const {
        uint32_t i = 0;
        for(auto& p: objects_) {
            auto ptr = p.second;
            callback(i++, SmartPointerConverter::convert(ptr));
        }
    }

    ObjectTypePtr find_object(const std::string& name) const {
        for(auto& p: objects_) {
            if(p.second->name() == name) {
                return p.second;
            }
        }

        return ObjectTypePtr();
    }

protected:
    uint32_t next_id() {
        return IDCounter<ObjectType>::next_id();
    }

    typedef std::shared_ptr<ObjectType> ObjectTypeInternalPtrType;

    std::unordered_map<
        IDType, ObjectTypeInternalPtrType
    > objects_;

    sig::signal<void (ObjectType&, IDType)> signal_post_create_;
    sig::signal<void (ObjectType&, IDType)> signal_pre_destroy_;

    virtual void on_make(IDType id) {
        _S_UNUSED(id);
    }

    virtual void on_get(IDType id) {
        _S_UNUSED(id);
    }

    virtual void on_destroy(IDType id) {
        _S_UNUSED(id);
    }
};


template<typename T>
struct ToSharedPtr {
    static std::shared_ptr<T> convert(const std::shared_ptr<T>& ptr) {
        return ptr;
    }
};

template<typename T>
struct ToDefaultInitPtr {
    static default_init_ptr<T> convert(const std::shared_ptr<T>& ptr) {
        return ptr.get();
    }
};


}

template<typename IDType, typename ObjectType, bool RefCounted>
class ObjectManager;

template<typename IDType, typename ObjectType>
class ObjectManager<IDType, ObjectType, false>:
    public _object_manager_impl::ObjectManagerBase<
        IDType, ObjectType, default_init_ptr<ObjectType>,
        _object_manager_impl::ToDefaultInitPtr<ObjectType>
    > {

public:
    typedef typename _object_manager_impl::ObjectManagerBase<
        IDType, ObjectType, default_init_ptr<ObjectType>, _object_manager_impl::ToDefaultInitPtr<ObjectType>
    > parent_class;

    typedef typename parent_class::ObjectTypePtr ObjectTypePtr;
    typedef typename parent_class::object_type object_type;

    void update() override {}
};

enum GarbageCollectMethod {
    GARBAGE_COLLECT_NEVER,
    GARBAGE_COLLECT_PERIODIC
};

template<typename IDType, typename ObjectType>
class ObjectManager<IDType, ObjectType, true>:
    public _object_manager_impl::ObjectManagerBase<
        IDType, ObjectType, std::shared_ptr<ObjectType>,
        _object_manager_impl::ToSharedPtr<ObjectType>
    > {

public:
    typedef typename _object_manager_impl::ObjectManagerBase<
        IDType, ObjectType, std::shared_ptr<ObjectType>, _object_manager_impl::ToSharedPtr<ObjectType>
    > parent_class;

    typedef typename parent_class::ObjectTypePtr ObjectTypePtr;
    typedef typename parent_class::object_type object_type;

    void update() override {
        for(auto it = this->objects_.begin(); it != this->objects_.end();) {
            ObjMeta meta = object_metas_.at(it->first);
            bool collect = meta.collection_method == GARBAGE_COLLECT_PERIODIC;

            if(collect && it->second.unique()) {
                // The user accessed this, and GC is enabled, and now there is only the
                // single ref left
                on_destroy(it->first);
                it = this->objects_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void set_garbage_collection_method(IDType id, GarbageCollectMethod method) {
        auto& meta = object_metas_.at(id);
        meta.collection_method = method;
        if(method != GARBAGE_COLLECT_NEVER) {
            meta.created = std::chrono::system_clock::now();
        }
    }

private:
    typedef std::chrono::time_point<std::chrono::system_clock> date_time;

    struct ObjMeta {
        ObjMeta():
            created(std::chrono::system_clock::now()) {}

        GarbageCollectMethod collection_method = GARBAGE_COLLECT_PERIODIC;
        date_time created;
    };

    std::unordered_map<IDType, ObjMeta> object_metas_;

    void on_make(IDType id) override {
        object_metas_.insert(std::make_pair(id, ObjMeta()));
    }

    void on_destroy(IDType id) override {
        S_DEBUG("Garbage collecting {0}", id);

        object_metas_.erase(id);
    }
};


}
