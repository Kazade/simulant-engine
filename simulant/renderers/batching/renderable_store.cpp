
#include "renderable_store.h"

namespace smlt {

RenderableFactory* RenderableStore::new_factory() {
    active_factories_.push_back(std::make_shared<RenderableFactory>(this));
    return active_factories_.back().get();
}

void RenderableStore::clear() {
    active_factories_.clear();
    renderables_.clear();
}

}
