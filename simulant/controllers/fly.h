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

#pragma once

#include <deque>
#include "./controller.h"
#include "../window.h"

#include "../interfaces/transformable.h"

namespace smlt {
namespace controllers {

class Fly:
    public ControllerWithInput,
    public Managed<Fly> {

public:
    Fly(Controllable* container, const Property<smlt::SceneBase, smlt::Window>& window):
        Fly(container, window.get()) {}

    Fly(Controllable* container, Window* window):
        ControllerWithInput(window->input.get()) {

        object_ = dynamic_cast<Transformable*>(container);

        if(!object_) {
            throw std::logic_error("Tried to attach FlyController to something which wasn't an object");
        }
    }

    const std::string name() const override { return "Fly by Keyboard"; }

private:
    void late_update(float dt) override {
        object_->move_forward_by(input->axis_value("Vertical") * 600.0 * dt);
        object_->rotate_global_y_by(Degrees(input->axis_value("Horizontal") * 50.0 * dt));
    }

    Transformable* object_ = nullptr;
};

}
}
