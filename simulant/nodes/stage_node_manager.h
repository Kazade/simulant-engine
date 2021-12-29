#pragma once

#include "../generic/containers/polylist.h"

#include "actor.h"
#include "camera.h"
#include "geom.h"
#include "light.h"
#include "particle_system.h"
#include "sprite.h"
#include "ui/button.h"
#include "ui/image.h"
#include "ui/label.h"
#include "ui/progress_bar.h"
#include "ui/frame.h"
#include "ui/keyboard.h"

#define STAGE_NODE_MANAGER_DEBUG 0

namespace smlt {


template<typename PoolType, typename IDType, typename T, typename ...Subtypes>
class StageNodeManager {
public:
    typedef StageNodeManager<PoolType, IDType, T, Subtypes...> this_type;

    StageNodeManager(PoolType* pool):
        pool_(pool) {}

    ~StageNodeManager() {
        clear();
    }

    const PoolType* pool() const {
        return pool_;
    }

    template<typename Derived, typename... Args>
    Derived* make_as(Args&&... args) {
        auto pair = pool_->template create<Derived>(
            std::forward<Args>(args)...
        );

        Derived* derived = pair.first;
        assert(derived);

        derived->_overwrite_id(IDType(pair.second));
        derived->_bind_id_pointer(derived);

        if(!derived->init()) {
            derived->clean_up();
            pool_->erase(pool_->find(pair.second));
            throw InstanceInitializationError();
        }

        objects_.push_back(derived);

        return derived;
    }

    template<typename... Args>
    T* make(Args&&... args) {
        return make_as<T>(std::forward<Args>(args)...);
    }

    bool contains(const IDType& id) const {
        return bool(get(id));
    }

    T* get(const IDType& id) const {
        StageNode* node = (*pool_)[id.value()];
        T* result = dynamic_cast<T*>(node);
#if STAGE_NODE_MANAGER_DEBUG
        assert((node && result) || (!node && !result));
#endif
        return result;
    }

    T* clone(const IDType& id, this_type* target_manager=nullptr) {
        T* source = get(id);
        return target_manager->make(*source);
    }

    bool destroy(const IDType& id) {
        auto it = pool_->find(id.value());
        if(it != pool_->end()) {
            /* Ensure we fire the destroyed signal */
            if(!(*it)->destroyed_) {
                (*it)->signal_destroyed()();
                (*it)->destroyed_ = true;
            }

            queued_for_destruction_.insert(id);
            return true;
        } else {
            return false;
        }
    }

    void clear() {
        destroy_all();
        clean_up();
    }

    void destroy_all() {
        destroy_all_next_clean_ = true;
    }

    bool destroy_immediately(const IDType& id) {
        auto it = pool_->find(id.value());
        if(it != pool_->end()) {
            StageNode* node = *it;

            /* Ensure we fire the destroyed signal */
            if(!(*it)->destroyed_) {
                (*it)->signal_destroyed()();
                (*it)->destroyed_ = true;
            }

            T* a = dynamic_cast<T*>(node);
            assert((node && a) || (!node && !a));

            a->clean_up();

            objects_.remove(a);
            pool_->erase(it);
            queued_for_destruction_.erase(id);
            return true;
        }
        return false;
    }

    void clean_up() {
        if(destroy_all_next_clean_) {
            destroy_all_next_clean_ = false;
            for(auto ptr: objects_) {
                ptr->clean_up();
                pool_->erase(pool_->find(ptr->id().value()));
            }
            objects_.clear();
        } else {
            auto queued = queued_for_destruction_;
            for(auto i: queued) {
                destroy_immediately(i);
            }
        }
        queued_for_destruction_.clear();
    }

    std::size_t size() const {
        return objects_.size();
    }

    typename std::list<T*>::iterator begin() {
        return objects_.begin();
    }

    typename std::list<T*>::iterator end() {
        return objects_.end();
    }

private:
    PoolType* pool_ = nullptr;
    std::list<T*> objects_;
    std::set<IDType> queued_for_destruction_;
    bool destroy_all_next_clean_ = false;
};

}
