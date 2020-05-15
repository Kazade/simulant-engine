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

namespace smlt {

typedef Polylist<
    StageNode,
    Actor/*, Camera, Geom, Light, ParticleSystem, Sprite,
    ui::Button, ui::Image, ui::Label, ui::ProgressBar*/
> StageNodePool;

template<typename IDType, typename T, typename ...Subtypes>
class StageNodeManager {
public:
    typedef StageNodeManager<IDType, T, Subtypes...> this_type;

    StageNodeManager(StageNodePool* pool):
        pool_(pool) {}

    const StageNodePool* pool() const {
        return pool_;
    }

    template<typename Derived, typename... Args>
    Derived* make_as(Args&&... args) {
        auto pair = pool_->create<Derived>(
            std::forward<Args>(args)...
        );

        Derived* derived = pair.first;
        assert(derived);

        derived->_overwrite_id(IDType(pair.second));
        derived->_bind_id_pointer(derived);

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
        assert((node && result) || (!node && !result));
        return result;
    }

    T* clone(const IDType& id, this_type* target_manager=nullptr) {
        T* source = get(id);
        return target_manager->make(*source);
    }

    bool destroy(const IDType& id) {
        if(queued_for_destruction_.count(id)) {
            return false;
        }

        queued_for_destruction_.insert(id);
        return true;
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
            T* a = dynamic_cast<T*>(node);
            assert((node && a) || (!node && !a));
            objects_.remove(a);
            pool_->erase(it);
            return true;
        }
        return false;
    }

    void clean_up() {
        if(destroy_all_next_clean_) {
            destroy_all_next_clean_ = false;
            for(auto ptr: objects_) {
                pool_->erase(pool_->find(ptr->id().value()));
            }
            objects_.clear();
        } else {
            for(auto i: queued_for_destruction_) {
                destroy_immediately(i);
            }
        }
    }

    bool is_marked_for_destruction(const IDType& id) const {
        return std::find(
            queued_for_destruction_.begin(),
            queued_for_destruction_.end(),
            id
        ) != queued_for_destruction_.end();
    }

    std::size_t size() const {
        return objects_.size();
    }
private:
    StageNodePool* pool_ = nullptr;
    std::list<T*> objects_;
    std::set<IDType> queued_for_destruction_;
    bool destroy_all_next_clean_ = false;
};

}
