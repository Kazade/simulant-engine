#include "light.h"

namespace kglt {

void Light::set_attenuation(float range, float constant, float linear, float quadratic) {
    range_ = range;
    const_attenuation_ = constant;
    linear_attenuation_ = linear;
    quadratic_attenuation_ = quadratic;
}

}
