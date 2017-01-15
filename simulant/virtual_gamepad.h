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

#ifndef VIRTUAL_GAMEPAD_H
#define VIRTUAL_GAMEPAD_H

#include <unordered_map>
#include "deps/kazsignal/kazsignal.h"
#include "generic/managed.h"
#include "types.h"

#include "input_controller.h"

namespace smlt {

struct Dimensions {
    float top;
    float left;
    float width;
    float height;
};

class VirtualGamepad : public Managed<VirtualGamepad> {
public:
    VirtualGamepad(WindowBase& window, VirtualDPadDirections directions, int button_count);

    bool init();
    void cleanup();

    void flip();

    sig::signal<void (int)>& signal_button_down() { return signal_button_down_; }
    sig::signal<void (int)>& signal_button_up() { return signal_button_up_; }

    sig::signal<void (HatPosition)>& signal_hat_changed() { return signal_hat_changed_; }
    sig::signal<void (JoypadAxis, int)>& signal_axis_changed() { return signal_axis_changed_; }

    Dimensions button_dimensions(int button);

    void _disable_rendering();
private:
    WindowBase& window_;
    VirtualDPadDirections directions_ = VIRTUAL_DPAD_DIRECTIONS_TWO;
    int button_count_ = 0;

    std::unordered_map<int, std::set<int>> button_touches_;
    std::unordered_map<std::string, std::set<int>> dpad_button_touches_;

    StageID stage_;
    CameraID camera_id_;
    PipelineID pipeline_id_;

    sig::signal<void (int)> signal_button_down_;
    sig::signal<void (int)> signal_button_up_;
    sig::signal<void (HatPosition)> signal_hat_changed_;
    sig::signal<void (JoypadAxis, int)> signal_axis_changed_;
};

}
#endif // VIRTUAL_GAMEPAD_H
