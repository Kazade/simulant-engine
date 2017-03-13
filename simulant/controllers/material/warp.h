/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WARP_H
#define WARP_H

#include "../../material.h"
#include "../controller.h"
#include "../../generic/managed.h"

namespace smlt {
namespace controllers {
namespace material {

class Warp:
    public MaterialController,
    public Managed<Warp> {

public:
    Warp(Controllable* material):
        MaterialController(dynamic_cast<Material*>(material)) {

    }

    const std::string name() const { return "Warped Material"; }

private:
    void update(float dt) override;
    double time_ = 0.0f;
};

}
}
}

#endif // WARP_H
