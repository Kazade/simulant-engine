#include "stage.h"
#include "light.h"

namespace smlt {

Light::Light(LightID lid, Stage* stage):
    ParentSetterMixin<MoveableObject>(stage),
    generic::Identifiable<LightID>(lid),
    type_(LIGHT_TYPE_POINT),
    range_(100.0) {

    set_ambient(smlt::Colour(0.3f, 0.3f, 0.3f, 1.0f));
    set_diffuse(smlt::Colour(0.9f, 0.9f, 1.0f, 1.0f));
    set_specular(smlt::Colour(0.01f, 0.01f, 0.01f, 1.0f));
    set_attenuation_from_range(100.0);
}

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
    const_attenuation_ = 1.0;
    linear_attenuation_ = 4.5 / range;
    quadratic_attenuation_ = 75.0 / (range * range);
}

void Light::ask_owner_for_destruction() {
    stage->delete_light(id());
}

}
