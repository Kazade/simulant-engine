#include "uniform_manager.h"

namespace smlt {

void UniformManager::register_auto(ShaderAvailableAuto uniform, const std::string &var_name) {
    auto_uniforms_[uniform] = var_name;
}

}
