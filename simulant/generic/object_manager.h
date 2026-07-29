#pragma once

#include <chrono>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "../core/memory_log.h"
#include "../core/stage_node_id.h"

#include "../logging.h"
#include "../path.h"
#include "../signals/signal.h"
#include "../macros.h"

namespace smlt {

const bool DONT_REFCOUNT = false;
const bool DO_REFCOUNT = true;

namespace _object_manager_impl {

/* C++17-compatible detection of whether T exposes asset_type_name() (i.e.
 * is Asset-derived), used to gate memory-log calls to managers of actual
 * assets and skip them (at compile time) for other ObjectManager users
 * such as GPUProgram. */
template<typename T, typename = void>
struct HasAssetTypeName: std::false_type {};

template<typename T>
struct HasAssetTypeName<
    T, std::void_t<decltype(std::declval<const T&>().asset_type_name())>>:
    std::true_type {};

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
struct ToRawPtr {
    static T* convert(const std::shared_ptr<T>& ptr) {
        return ptr.get();
    }
};
}

template<typename IDType, typename ObjectType, bool RefCounted>
class ObjectManager;

template<typename IDType, typename ObjectType>
class ObjectManager<IDType, ObjectType, false>:
    public _object_manager_impl::ObjectManagerBase<
        IDType, ObjectType, ObjectType*,
        _object_manager_impl::ToRawPtr<ObjectType>> {

public:
    typedef typename _object_manager_impl::ObjectManagerBase<
        IDType, ObjectType, ObjectType*,
        _object_manager_impl::ToRawPtr<ObjectType>>
        parent_class;

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
            auto use_count = it->second.use_count();

            if(collect && use_count <= 1) {
                /* FIXME: use_count isn't thread safe */
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

    /* Returns an already-registered object with this source path, or nullptr
     * if there isn't one (or the object it referred to was since destroyed).
     * An empty source path never matches anything. */
    ObjectTypePtr get_by_source(const Path& source) const {
        if(source.str().empty()) {
            return ObjectTypePtr();
        }

        auto it = source_index_.find(source);
        if(it == source_index_.end()) {
            return ObjectTypePtr();
        }

        return this->get(it->second);
    }

    /* Records that the object with the given id was loaded from source,
     * so future get_by_source() calls can find it. No-op for an empty
     * source path. */
    void register_source(IDType id, const Path& source) {
        if(source.str().empty()) {
            return;
        }

        object_metas_.at(id).source = source;
        source_index_[source] = id;
    }

private:
    typedef std::chrono::time_point<std::chrono::system_clock> date_time;

    struct ObjMeta {
        ObjMeta():
            created(std::chrono::system_clock::now()) {}

        GarbageCollectMethod collection_method = GARBAGE_COLLECT_PERIODIC;
        date_time created;
        Path source;
    };

    std::unordered_map<IDType, ObjMeta> object_metas_;
    std::unordered_map<Path, IDType> source_index_;

    void on_make(IDType id) override {
        object_metas_.insert(std::make_pair(id, ObjMeta()));

        /* Only ObjectType's deriving from Asset expose asset_type_name(),
         * so this is skipped entirely (at compile time) for managers of
         * non-asset objects (e.g. GPUProgram). */
        if constexpr(_object_manager_impl::HasAssetTypeName<ObjectType>::value) {
            auto obj = this->get(id);
            log_memory_event(MEMORY_LOG_EVENT_ALLOC, obj->asset_type_name(),
                              (uint64_t)obj->id(), obj->source().str(),
                              obj->name());
        }
    }

    void on_destroy(IDType id) override {
        if constexpr(_object_manager_impl::HasAssetTypeName<ObjectType>::value) {
            auto obj = this->get(id);
            if(obj) {
                log_memory_event(MEMORY_LOG_EVENT_DEALLOC,
                                  obj->asset_type_name(), (uint64_t)obj->id(),
                                  obj->source().str(), obj->name());
            }
        }

        S_DEBUG("Garbage collecting {0}", id);

        auto it = object_metas_.find(id);
        if(it != object_metas_.end()) {
            if(!it->second.source.str().empty()) {
                source_index_.erase(it->second.source);
            }
            object_metas_.erase(it);
        }
    }
};


}
