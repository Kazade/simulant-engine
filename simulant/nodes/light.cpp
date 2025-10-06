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

Light::Light(Scene* owner, StageNodeType type) :
    ContainerNode(owner, type), type_(LIGHT_TYPE_POINT) {}

void Light::set_type(LightType type) {
    type_ = type;

    /* Don't cull directional lights */
    set_cullable(type_ != LIGHT_TYPE_DIRECTIONAL);
}

bool Light::on_create(Params args) {
    Color c = args.get<FloatArray>("color").value_or(smlt::Color::white());
    set_color(c);

    return true;
}
} // namespace smlt
