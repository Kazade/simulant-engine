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

typedef int8_t KeyboardID;
typedef int8_t MouseID;

#define STRONG_TYPEDEF(name, type) \
    typedef struct tag_ ## name { \
        type v; \
        type to_ ## type () const { return v; } \
        tag_ ## name () {} \
        explicit tag_ ## name (type i): v(i) {} \
        bool operator==(const tag_ ## name & rhs) const { return v == rhs.v; } \
    } (name)


/** GameControllerID represents the unique id of a *detected* controller
 *  it is *not* a zero-based index into the detected controller list */
STRONG_TYPEDEF(GameControllerID, int8_t);

/** GameControllerIndex is an index into the detected controller list, if
 *  a controller is unplugged, the IDs of all other controllers may change */
STRONG_TYPEDEF(GameControllerIndex, int8_t);

typedef int8_t MouseButtonID;
typedef int8_t JoystickHatID;

static const KeyboardID ALL_KEYBOARDS = -1;
static const MouseID ALL_MICE = -1;
static const GameControllerIndex ALL_GAME_CONTROLLERS = GameControllerIndex(-1);


struct GameControllerInfo {
    GameControllerID id;
    char name[32];
    uint8_t button_count;
    uint8_t axis_count;
    uint8_t hat_count;
    bool has_rumble;

    /* Space for each platform to store some limited data */
    union {
        uint32_t i;
        uint8_t  b[4];
    } platform_data;
};

struct KeyboardDeviceInfo {
    uint32_t id;
};

struct MouseDeviceInfo {
    uint32_t id;
    uint8_t button_count;
    uint8_t axis_count;
};


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
    JOYSTICK_AXIS_Y = JOYSTICK_AXIS_1,
    JOYSTICK_AXIS_XL = JOYSTICK_AXIS_0,
    JOYSTICK_AXIS_YL = JOYSTICK_AXIS_1,
    JOYSTICK_AXIS_XR = JOYSTICK_AXIS_2,
    JOYSTICK_AXIS_YR = JOYSTICK_AXIS_3,
    JOYSTICK_AXIS_LTRIGGER = JOYSTICK_AXIS_4,
    JOYSTICK_AXIS_RTRIGGER = JOYSTICK_AXIS_5,
};

enum JoystickHatAxis {
    JOYSTICK_HAT_AXIS_X,
    JOYSTICK_HAT_AXIS_Y
};

/* All IDs here (aside INVALID) match up with those
 * that SDL uses. This is for convenience */
enum JoystickButton : int8_t {
    JOYSTICK_BUTTON_INVALID = -1,
    JOYSTICK_BUTTON_A,
    JOYSTICK_BUTTON_B,
    JOYSTICK_BUTTON_X,
    JOYSTICK_BUTTON_Y,
    JOYSTICK_BUTTON_BACK,
    JOYSTICK_BUTTON_GUIDE,
    JOYSTICK_BUTTON_START,
    JOYSTICK_BUTTON_LEFT_STICK,
    JOYSTICK_BUTTON_RIGHT_STICK,
    JOYSTICK_BUTTON_LEFT_SHOULDER,
    JOYSTICK_BUTTON_RIGHT_SHOULDER,
    JOYSTICK_BUTTON_DPAD_UP,
    JOYSTICK_BUTTON_DPAD_DOWN,
    JOYSTICK_BUTTON_DPAD_LEFT,
    JOYSTICK_BUTTON_DPAD_RIGHT,
    JOYSTICK_BUTTON_DPAD2_UP,
    JOYSTICK_BUTTON_DPAD2_DOWN,
    JOYSTICK_BUTTON_DPAD2_LEFT,
    JOYSTICK_BUTTON_DPAD2_RIGHT,
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

class InputState;

class GameController {
    friend class InputState;

    GameController() = default;
    GameController(InputState* parent, GameControllerID id):
        parent_(parent),
        id_(id) {}

public:
    GameControllerID id() const {
        return id_;
    }

    bool has_rumble_effect() const;
    bool start_rumble(float low_rumble, float high_rumble, const smlt::Seconds& duration);
    void stop_rumble();

    bool button_state(JoystickButton button) const;
    float axis_state(JoystickAxis axis) const;
    HatPosition hat_state(JoystickHatID hat) const;

    const uint8_t* platform_data() const { return &platform_data_.b[0]; }
private:
    InputState* parent_ = nullptr;
    GameControllerID id_ = GameControllerID(-1);

    uint8_t button_count = 0;
    uint8_t axis_count = 0;
    uint8_t hat_count = 0;

    bool has_rumble_ = false;
    union {
        uint32_t i;
        uint8_t  b[4];
    } platform_data_;

    bool buttons[JOYSTICK_BUTTON_MAX] = {0};
    float axises[JOYSTICK_AXIS_MAX] = {0};
    HatPosition hats[MAX_JOYSTICK_HATS] = {HAT_POSITION_CENTERED};
};


class InputState:
    public RefCounted<InputState> {

public:
    InputState(Window* window):
        window_(window) {}

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

    void _update_game_controllers(const std::vector<GameControllerInfo>& device_info);

    void _handle_key_down(KeyboardID keyboard_id, KeyboardCode code);
    void _handle_key_up(KeyboardID keyboard_id, KeyboardCode code);

    void _handle_mouse_motion(MouseID mouse_id, uint32_t x, uint32_t y, int32_t xrel, int32_t yrel);
    void _handle_mouse_down(MouseID mouse_id, MouseButtonID button_id);
    void _handle_mouse_up(MouseID mouse_id, MouseButtonID button_id);

    // value must be a value between -1.0f and 1.0f!
    void _handle_joystick_axis_motion(GameControllerID joypad_id, JoystickAxis axis, float value);

    void _handle_joystick_button_down(GameControllerID joypad_id, JoystickButton button);
    void _handle_joystick_button_up(GameControllerID joypad_id, JoystickButton button);
    void _handle_joystick_hat_motion(GameControllerID joypad_id, JoystickHatID hat_id, HatPosition position);

    // Public state accessor functions
    bool keyboard_key_state(KeyboardID keyboard_id, KeyboardCode code) const;

    bool mouse_button_state(MouseID mouse_id, MouseButtonID button) const;

    float mouse_axis_state(MouseID mouse_id, MouseAxis axis) const;
    Vec2 mouse_position(MouseID mouse_id) const;

    GameController* game_controller_by_id(GameControllerID id);
    const GameController* game_controller_by_id(GameControllerID id) const;
    GameController* game_controller(GameControllerIndex id);
    GameControllerIndex game_controller_index_from_id(GameControllerID id) const;

    std::size_t game_controller_count() const { return joystick_count_; }
    std::size_t keyboard_count() const { return keyboard_count_; }
    std::size_t mouse_count() const { return mouse_count_; }

    JoystickAxis linked_axis(GameControllerID id, JoystickAxis axis);

private:
    friend class GameController;

    Window* window_ = nullptr;

    bool joystick_button_state(GameControllerID joystick_id, JoystickButton button) const;
    float joystick_axis_state(GameControllerID joystick_id, JoystickAxis axis) const;
    HatPosition joystick_hat_state(GameControllerID joystick_id, JoystickHatID hat) const;


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

    uint8_t joystick_count_ = 0;
    GameController joysticks_[4];
};

}
#endif // INPUT_CONTROLLER_H
