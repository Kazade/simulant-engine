#include "animation_controller.h"

namespace smlt {

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
