#include "scene.h"
#include "light.h"

namespace kglt {

/**
    Sets the attenuation and the range of the light. The range doesn't have any
    direct effect on the brightness on the light, it simply is a cut-off -
    outside of which objects will not be lit by the light.

    e.g. the larger the range, the more objects will be rendered for this light
    even if the light would never reach them.
*/
void Light::set_attenuation(float range, float constant, float linear, float quadratic) {
    range_ = range;
    const_attenuation_ = constant;
    linear_attenuation_ = linear;
    quadratic_attenuation_ = quadratic;
}

/**
 * @brief Light::set_attenuation_from_range
 * @param range - The range of the lights brightness
 *
 * This is a shortcut function that calculates attenuation so that at the
 * light position the brightness is 100% and at a distance of 'range' from the
 * light the brightness is 0%
 */
void Light::set_attenuation_from_range(float range) {
    range_ = range;
    const_attenuation_ = 0.5;
    linear_attenuation_ = 4.5 / range;
    quadratic_attenuation_ = 75.0 / (range * range);
}

void Light::destroy() {
    subscene().delete_light(id());
}

}
