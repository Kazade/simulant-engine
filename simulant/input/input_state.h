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

#ifndef INPUT_CONTROLLER_H
#define INPUT_CONTROLLER_H

#include <cstdint>
#include <vector>
#include <map>
#include <functional>
#include <set>
#include <cassert>

#include "../keycodes.h"
#include "../types.h"

#include "../signals/signal.h"
#include "../generic/managed.h"
#include "../generic/identifiable.h"
#include "../generic/unique_id.h"

namespace smlt {

class InputState;

struct JoystickDeviceInfo {
    uint32_t id;
    std::string name;
    uint8_t button_count;
    uint8_t axis_count;
    uint8_t hat_count;
};

struct KeyboardDeviceInfo {
    uint32_t id;
};

struct MouseDeviceInfo {
    uint32_t id;
    uint8_t button_count;
    uint8_t axis_count;
};


typedef int8_t KeyboardID;
typedef int8_t MouseID;
typedef int8_t JoystickID;
typedef int8_t MouseButtonID;
typedef int8_t JoystickHatID;

static const KeyboardID ALL_KEYBOARDS = -1;
static const MouseID ALL_MICE = -1;
static const JoystickID ALL_JOYSTICKS = -1;

enum MouseAxis {
    MOUSE_AXIS_INVALID = -1,
    MOUSE_AXIS_0,
    MOUSE_AXIS_1,
    MOUSE_AXIS_2,
    MOUSE_AXIS_3,
    MOUSE_AXIS_MAX,
    MOUSE_AXIS_X = MOUSE_AXIS_0,
    MOUSE_AXIS_Y = MOUSE_AXIS_1
};

enum JoystickAxis {
    JOYSTICK_AXIS_INVALID = -1,
    JOYSTICK_AXIS_0,
    JOYSTICK_AXIS_1,
    JOYSTICK_AXIS_2,
    JOYSTICK_AXIS_3,
    JOYSTICK_AXIS_4,
    JOYSTICK_AXIS_5,
    JOYSTICK_AXIS_6,
    JOYSTICK_AXIS_7,
    JOYSTICK_AXIS_MAX,
    JOYSTICK_AXIS_X = JOYSTICK_AXIS_0,
    JOYSTICK_AXIS_Y = JOYSTICK_AXIS_1
};

enum JoystickHatAxis {
    JOYSTICK_HAT_AXIS_X,
    JOYSTICK_HAT_AXIS_Y
};

/* All IDs here (aside INVALID) match up with those
 * that SDL uses. This is for convenience */
enum JoystickButton {
    JOYSTICK_BUTTON_INVALID = -1,
    JOYSTICK_BUTTON_A = 1,
    JOYSTICK_BUTTON_B = 2,
    JOYSTICK_BUTTON_X = 3,
    JOYSTICK_BUTTON_Y = 4,
    JOYSTICK_BUTTON_BACK = 5,
    JOYSTICK_BUTTON_GUIDE = 6,
    JOYSTICK_BUTTON_START = 7,
    JOYSTICK_BUTTON_LEFT_STICK = 8,
    JOYSTICK_BUTTON_RIGHT_STICK = 9,
    JOYSTICK_BUTTON_LEFT_SHOULDER = 10,
    JOYSTICK_BUTTON_RIGHT_SHOULDER = 11,
    JOYSTICK_BUTTON_DPAD_UP = 12,
    JOYSTICK_BUTTON_DPAD_DOWN = 13,
    JOYSTICK_BUTTON_DPAD_LEFT = 14,
    JOYSTICK_BUTTON_DPAD_RIGHT = 15,
    JOYSTICK_BUTTON_MAX
};

const static std::size_t MAX_MOUSE_BUTTONS = 16u;
const static std::size_t MAX_JOYSTICK_HATS = 4u;
const static std::size_t MAX_DEVICE_TYPE_COUNT = 4u;

enum HatPosition {
    HAT_POSITION_CENTERED = 0,
    HAT_POSITION_UP = 1,
    HAT_POSITION_RIGHT = 2,
    HAT_POSITION_DOWN = 4,
    HAT_POSITION_LEFT = 8,
    HAT_POSITION_RIGHT_UP = HAT_POSITION_RIGHT | HAT_POSITION_UP,
    HAT_POSITION_RIGHT_DOWN = HAT_POSITION_RIGHT | HAT_POSITION_DOWN,
    HAT_POSITION_LEFT_UP = HAT_POSITION_LEFT | HAT_POSITION_UP,
    HAT_POSITION_LEFT_DOWN = HAT_POSITION_LEFT | HAT_POSITION_DOWN
};

class InputState:
    public RefCounted<InputState> {

public:
    void pre_update(float dt);
    void update(float dt);

    /* These methods should be called by BaseWindow subclasses when the OS sends the corresponding
     * event. You should not call these unless you are implementing support for a new platform!
     */

    void _update_mouse_devices(const std::vector<MouseDeviceInfo>& device_info) {
        mouse_count_ = std::min(device_info.size(), MAX_DEVICE_TYPE_COUNT);
        for(decltype(mouse_count_) i = 0; i < mouse_count_; ++i) {
            mice_[i].button_count = device_info[i].button_count;
            mice_[i].axis_count = device_info[i].axis_count;
        }
    }

    void _update_keyboard_devices(const std::vector<KeyboardDeviceInfo>& device_info) {
        keyboard_count_ = std::min(device_info.size(), MAX_DEVICE_TYPE_COUNT);
    }

    void _update_joystick_devices(const std::vector<JoystickDeviceInfo>& device_info);

    void _handle_key_down(KeyboardID keyboard_id, KeyboardCode code);
    void _handle_key_up(KeyboardID keyboard_id, KeyboardCode code);

    void _handle_mouse_motion(MouseID mouse_id, uint32_t x, uint32_t y, int32_t xrel, int32_t yrel);
    void _handle_mouse_down(MouseID mouse_id, MouseButtonID button_id);
    void _handle_mouse_up(MouseID mouse_id, MouseButtonID button_id);

    // value must be a value between -1.0f and 1.0f!
    void _handle_joystick_axis_motion(JoystickID joypad_id, JoystickAxis axis, float value);

    void _handle_joystick_button_down(JoystickID joypad_id, JoystickButton button);
    void _handle_joystick_button_up(JoystickID joypad_id, JoystickButton button);
    void _handle_joystick_hat_motion(JoystickID joypad_id, JoystickHatID hat_id, HatPosition position);

    // Public state accessor functions
    bool keyboard_key_state(KeyboardID keyboard_id, KeyboardCode code) const;

    bool mouse_button_state(MouseID mouse_id, MouseButtonID button) const;

    float mouse_axis_state(MouseID mouse_id, MouseAxis axis) const;
    Vec2 mouse_position(MouseID mouse_id) const;

    bool joystick_button_state(JoystickID joystick_id, JoystickButton button) const;
    float joystick_axis_state(JoystickID joystick_id, JoystickAxis axis) const;
    HatPosition joystick_hat_state(JoystickID joystick_id, JoystickHatID hat) const;

    std::size_t joystick_count() const { return joystick_count_; }
    std::size_t keyboard_count() const { return keyboard_count_; }
    std::size_t mouse_count() const { return mouse_count_; }

    void init_virtual_joypad();

    JoystickAxis linked_axis(JoystickID id, JoystickAxis axis);
private:
    struct KeyboardState {
        bool keys[MAX_KEYBOARD_CODES] = {0};
    };

    uint8_t keyboard_count_ = 0;
    KeyboardState keyboards_[4];

    struct MouseState {
        uint8_t button_count = 0;
        uint8_t axis_count = 0;

        bool buttons[MAX_MOUSE_BUTTONS] = {0};
        float axises[MOUSE_AXIS_MAX] = {0};

        uint32_t x = 0;
        uint32_t y = 0;
    };

    uint8_t mouse_count_ = 0;
    MouseState mice_[4];

    struct JoystickState {
        uint8_t button_count = 0;
        uint8_t axis_count = 0;
        uint8_t hat_count = 0;

        bool buttons[JOYSTICK_BUTTON_MAX] = {0};
        float axises[JOYSTICK_AXIS_MAX] = {0};
        HatPosition hats[MAX_JOYSTICK_HATS] = {HAT_POSITION_CENTERED};
    };

    uint8_t joystick_count_ = 0;
    JoystickState joysticks_[4];
};

}
#endif // INPUT_CONTROLLER_H
