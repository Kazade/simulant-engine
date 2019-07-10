#pragma once

#include <vector>
#include "renderable.h"

namespace smlt {

/* Used to build a list of renderables while keeping
 * them all in contiguous memory to prevent continuous allocations
 *
 * Usage:
 *
 * auto factory = store_->new_factory();
 * factory->push_renderable(my_renderable);
 *
 * auto another = store_->new_factory();
 * factory->push_renderable(other_renderable);
 *
 * // ... At the end of the frame
 *
 * store_->clear();
 */

class RenderableFactory;

class RenderableStore {
public:
    RenderableFactory* new_factory();

    void clear();

    template<typename Func>
    void each_renderable(Func func) {
        for(auto& r: renderables_) {
            func(&r);
        }
    }
private:
    std::vector<std::shared_ptr<RenderableFactory>> active_factories_;
    std::vector<Renderable> renderables_;

    friend class RenderableFactory;
};

class RenderableFactory {
public:
    RenderableFactory(RenderableStore* store):
        store_(store) {

    }

    void push_renderable(const Renderable& r) {
        assert(is_valid());
        store_->renderables_.push_back(std::move(r));
        pushed_.push_back(store_->renderables_.size() - 1);
    }

    template<typename Func>
    void each_pushed(Func cb) {
        // FIXME: Is there a faster way? Likely that renderables
        // will be pushed in order
        for(auto idx: pushed_) {
            cb(&store_->renderables_[idx]);
        }
    }

    bool is_valid() const {
        for(auto& renderable: store_->active_factories_) {
            if(renderable.get() == this) {
                return true;
            }
        }

        return false;
    }

private:
    RenderableStore* store_ = nullptr;
    std::vector<std::size_t> pushed_;
};


}
