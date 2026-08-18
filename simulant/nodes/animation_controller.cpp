#include "animation_controller.h"

#include "armature.h"

namespace smlt {

void AnimationController::update_armatures() {
    for(auto& node: base()->each_descendent()) {
        if(node.node_type() == Armature::Meta::node_type) {
            static_cast<Armature*>(&node)->update_skinning();
        }
    }
}

std::pair<std::size_t, std::size_t>
    AnimationData::find_times_indices(float t) const {
    if(t < 0.0f) {
        return std::make_pair(0u, 1u);
    }

    for(std::size_t i = 0u; i < times_.size(); ++i) {
        float time = times_[i];
        if(time > t) {
            return std::make_pair((i > 0) ? i - 1 : 0, i);
        }
    }

    return std::make_pair(times_.size() - 2, times_.size() - 1);
}

} // namespace smlt
