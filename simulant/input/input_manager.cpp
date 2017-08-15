#include "input.h"
#include "input_manager.h"

namespace smlt {

InputAxis* InputManager::new_axis(const std::string& name) {
    auto axis = InputAxis::create(name);
    axises_.push_back(axis);
    return axis.get();
}

AxisList InputManager::axises(const std::string& name) const {
    AxisList result;
    for(auto& axis: axises_) {
        result.push_back(axis.get());
    }
    return result;
}

void InputManager::each_axis(EachAxisCallback callback) {
    for(auto& axis: axises_) {
        callback(axis.get());
    }
}

void InputManager::delete_axises(const std::string& name) {
    axises_.erase(
        std::remove_if(axises_.begin(), axises_.end(),
            [&name](InputAxis::ptr axis) -> bool {
                return axis->name() == name;
            }
        ), axises_.end()
    );
}

void InputManager::delete_axis(InputAxis* axis) {
    axises_.erase(
        std::remove_if(axises_.begin(), axises_.end(),
            [axis](InputAxis::ptr ax) -> bool {
                return ax.get() == axis;
            }
        ), axises_.end()
    );
}

float InputManager::_calculate_value(InputAxis *axis) const {

}

float InputManager::axis_value(const std::string& name) const {
    auto f = 0.0f;
    for(auto axis: axises(name)) {
        auto v = _calculate_value(axis);

        // Return the result with the greatest overall value (positive or negative)
        if(fabs(v) > fabs(f)) {
            f = v;
        }
    }

    return f;
}


}
