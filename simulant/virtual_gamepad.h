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

#ifndef VIRTUAL_GAMEPAD_H
#define VIRTUAL_GAMEPAD_H

#include <unordered_map>
#include "signals/signal.h"
#include "generic/managed.h"
#include "generic/property.h"
#include "types.h"

#include "input/input_state.h"

namespace smlt {

class VirtualGamepad:
    public RefCounted<VirtualGamepad> {
public:
    VirtualGamepad(Window& window, VirtualGamepadConfig config);

    bool init();
    void clean_up();

    void flip();

    sig::signal<void (int)>& signal_button_down() { return signal_button_down_; }
    sig::signal<void (int)>& signal_button_up() { return signal_button_up_; }

    sig::signal<void (HatPosition)>& signal_hat_changed() { return signal_hat_changed_; }
    sig::signal<void (JoystickAxis, int)>& signal_axis_changed() { return signal_axis_changed_; }

    AABB button_bounds(int button);

private:
    Window& window_;
    VirtualGamepadConfig config_ = VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS;

    std::vector<ui::Button*> buttons_;

    StagePtr stage_;
    StageID stage_id_;

    CameraPtr camera_;
    PipelinePtr pipeline_;

    sig::signal<void (int)> signal_button_down_;
    sig::signal<void (int)> signal_button_up_;
    sig::signal<void (HatPosition)> signal_hat_changed_;
    sig::signal<void (JoystickAxis, int)> signal_axis_changed_;

    std::vector<sig::ScopedConnection> connections_;

    void _prepare_deletion();
    friend class Window; // For _prepare_deletion

public:
    S_DEFINE_PROPERTY(stage, &VirtualGamepad::stage_);

};

}
#endif // VIRTUAL_GAMEPAD_H
