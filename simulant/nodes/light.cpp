//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "light.h"
#include "../scenes/scene.h"

namespace smlt {

// Apparently this is the color of a high noon sun (color temp 5400 - 255, 255,
// 251)
const Color DEFAULT_LIGHT_COLOR = Color(1.0, 1.0, 251.0 / 255.0, 1.0);

Light::Light(Scene* owner, StageNodeType type) :
    ContainerNode(owner, type), type_(LIGHT_TYPE_POINT) {}

void Light::set_type(LightType type) {
    type_ = type;

    /* Don't cull directional lights */
    set_cullable(type_ != LIGHT_TYPE_DIRECTIONAL);
}

Color Light::global_ambient() const {
    return scene->lighting->ambient_light();
}

/**
    Sets the attenuation and the range of the light. The range doesn't have any
    direct effect on the brightness on the light, it simply is a cut-off -
    outside of which objects will not be lit by the light.

    e.g. the larger the range, the more objects will be rendered for this light
    even if the light would never reach them.
*/
void Light::set_attenuation(float range, float constant, float linear,
                            float quadratic) {
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
    const_attenuation_ = 1.0f;
    linear_attenuation_ = 4.5f / range;
    quadratic_attenuation_ = 75.0f / (range * range);
}

bool Light::on_create(const Params& args) {
    set_range(100.0f);
    set_attenuation_from_range(100.0);

    auto c = args.arg<Color>("color").value_or(DEFAULT_LIGHT_COLOR);
    set_ambient(c);
    set_diffuse(c);
    set_specular(c);

    return true;
}
} // namespace smlt
