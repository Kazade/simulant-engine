/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <deque>
#include "./behaviour.h"
#include "../window.h"

#include "../interfaces/transformable.h"

namespace smlt {
namespace behaviours {

class Fly:
    public BehaviourWithInput,
    public Managed<Fly> {

public:
    Fly(const Property<smlt::SceneBase, smlt::Window>& window):
        Fly(window.get()) {}

    Fly(Window* window):
        BehaviourWithInput(window->input.get()) {
    }

    const std::string name() const override { return "Fly by Keyboard"; }

private:
    void on_behaviour_added(Organism* controllable) override {
        stage_node_ = dynamic_cast<StageNode*>(controllable);
    }

    void on_behaviour_removed(Organism *controllable) override {
        stage_node_ = nullptr;
    }

    void late_update(float dt) override {
        if(!attached()) {
            return;
        }

        stage_node_->move_forward_by(input->axis_value("Vertical") * 600.0 * dt);
        stage_node_->rotate_global_y_by(Degrees(input->axis_value("Horizontal") * -50.0 * dt));
    }

    StageNode* stage_node_ = nullptr;
};

}
}
