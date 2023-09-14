#pragma once

#include <string>
#include "../generic/managed.h"
#include "../types.h"
#include "../keycodes.h"
#include "input_state.h"

namespace smlt {

/*
 * See here for more information on radial deadzones:
 * https://www.gamasutra.com/blogs/JoshSutphin/20130416/190541/Doing_Thumbstick_Dead_Zones_Right.php
 */
enum DeadZoneBehaviour {
    DEAD_ZONE_BEHAVIOUR_NONE,
    DEAD_ZONE_BEHAVIOUR_AXIAL,
    DEAD_ZONE_BEHAVIOUR_RADIAL
};

enum AxisType {
    AXIS_TYPE_UNSET,
    AXIS_TYPE_KEYBOARD_KEY,
    AXIS_TYPE_MOUSE_BUTTON,
    AXIS_TYPE_MOUSE_AXIS,
    AXIS_TYPE_JOYSTICK_BUTTON,
    AXIS_TYPE_JOYSTICK_AXIS,
    AXIS_TYPE_JOYSTICK_HAT
};

class InputAxis:
    public RefCounted<InputAxis> {
public:
    InputAxis(const std::string& name);

    void set_positive_keyboard_key(const KeyboardCode& key);
    void set_negative_keyboard_key(const KeyboardCode& key);
    KeyboardCode positive_keyboard_key() const { return positive_key_; }
    KeyboardCode negative_keyboard_key() const { return negative_key_; }

    void set_keyboard_source(KeyboardID keyboard);
    KeyboardID keyboard_source() const { return keyboard_source_; }

    void set_positive_mouse_button(MouseButtonID button);
    void set_negative_mouse_button(MouseButtonID button);
    MouseButtonID positive_mouse_button() const { return positive_mouse_button_; }
    MouseButtonID negative_mouse_button() const { return negative_mouse_button_; }
    void set_mouse_source(MouseID mouse);
    MouseID mouse_source() const { return mouse_source_; }

    void set_positive_joystick_button(JoystickButton button);
    void set_negative_joystick_button(JoystickButton button);
    JoystickButton positive_joystick_button() const { return positive_joystick_button_; }
    JoystickButton negative_joystick_button() const { return negative_joystick_button_; }
    void set_joystick_source(GameControllerIndex joystick);
    GameControllerIndex joystick_source() const { return joystick_source_; }

    void set_mouse_axis(MouseAxis axis);
    void set_joystick_axis(JoystickAxis axis);
    void set_joystick_hat_axis(JoystickHatID hat, JoystickHatAxis axis);

    /* For digital inputs only. The force multiplier (in units-per-second)
     * to change the axis value when pressed. Defaults to 3.0 (i.e.
     * it'll take 1/3rd second to hit an axis value of 1.0 */
    void set_force(float f);
    void set_return_speed(float ret);

    const std::string& name() const { return name_; }
    const AxisType& type() const { return type_; }

    float value(DeadZoneBehaviour dead_zone_behaviour=DEAD_ZONE_BEHAVIOUR_RADIAL) const;

    void set_dead_zone(float v) { dead_zone_ = v; dead_zone_reciprocal_ = 1.0f / (1.0f - dead_zone_); }
    float dead_zone() const { return dead_zone_; }

    void set_inversed(bool value=true);

private:
    std::string name_;

    AxisType type_ = AXIS_TYPE_UNSET;

    KeyboardID keyboard_source_ = ALL_KEYBOARDS;
    KeyboardCode positive_key_ = KEYBOARD_CODE_NONE;
    KeyboardCode negative_key_ = KEYBOARD_CODE_NONE;

    MouseID mouse_source_ = ALL_MICE;
    MouseButtonID positive_mouse_button_ = -1;
    MouseButtonID negative_mouse_button_ = -1;

    GameControllerIndex joystick_source_ = ALL_GAME_CONTROLLERS;
    JoystickButton positive_joystick_button_ = JOYSTICK_BUTTON_INVALID;
    JoystickButton negative_joystick_button_ = JOYSTICK_BUTTON_INVALID;

    MouseAxis mouse_axis_ = MOUSE_AXIS_INVALID;
    JoystickAxis joystick_axis_ = JOYSTICK_AXIS_INVALID;

    JoystickHatID joystick_hat_ = -1;
    JoystickHatAxis joystick_hat_axis_ = JOYSTICK_HAT_AXIS_X;

    float return_speed_ = 3.0f;
    float force_ = 3.0f;

    float value_ = 0.0f;
    float dead_zone_ = 0.001f;
    float dead_zone_reciprocal_ = 1.0f / (1.0f - dead_zone_);

    bool inversed_ = false;

    /* This is used where an axis has a counterpart
     * (e.g a joystick with X + Y). It's used when
     * calculating radial deadzones and stores the
     * normalized value of the other axis */
    float linked_value_ = 0.0;

    void set_type(AxisType type);

    friend class InputManager;
};

}
