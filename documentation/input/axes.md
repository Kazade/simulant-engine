# Input Axes

The `InputAxis` system is Simulant's abstraction for configurable input mappings. Instead of reading raw keyboard, mouse, or gamepad state directly, you define named axes that can be driven by any combination of devices. This makes it straightforward to support keyboard, mouse, and gamepad input with a single code path.

## Overview

An `InputAxis` represents a single control input that produces a value between `-1.0` and `1.0`. Each axis has a name (e.g. `"Horizontal"`, `"Fire1"`, `"MouseX"`) and can be backed by:

- Keyboard keys (positive and negative)
- Mouse buttons (positive and negative)
- Mouse movement axes
- Gamepad/joystick buttons (positive and negative)
- Gamepad/joystick analog axes (sticks, triggers)
- Gamepad/joystick hat (D-pad) positions

Multiple axes can share the same name. When reading a value by name, Simulant returns the **strongest** (highest absolute) value among all axes with that name. This lets you define separate keyboard and gamepad axes with the same name and have them "just work" without any conditional logic.

## Creating Axes

Axes are created through the `InputManager` using `new_axis(name)`. On creation the axis is non-functional; its `type()` is `AXIS_TYPE_UNSET` until you configure a source.

```cpp
// In your Scene's on_start() or similar setup method:
auto horizontal = input->new_axis("Horizontal");
horizontal->set_positive_keyboard_key(KEYBOARD_CODE_D);
horizontal->set_negative_keyboard_key(KEYBOARD_CODE_A);

auto vertical = input->new_axis("Vertical");
vertical->set_positive_keyboard_key(KEYBOARD_CODE_S);
vertical->set_negative_keyboard_key(KEYBOARD_CODE_W);

auto fire = input->new_axis("Fire1");
fire->set_positive_keyboard_key(KEYBOARD_CODE_SPACE);
```

Each call to `set_positive_*` or `set_negative_*` also sets the axis `type()` automatically.

### Gamepad Axes

For gamepad sticks and buttons you configure the axis similarly:

```cpp
// Left stick X-axis
auto horizontal_js = input->new_axis("Horizontal");
horizontal_js->set_type(AXIS_TYPE_JOYSTICK_AXIS);
horizontal_js->set_joystick_axis(JOYSTICK_AXIS_XL);

// Left stick Y-axis
auto vertical_js = input->new_axis("Vertical");
vertical_js->set_type(AXIS_TYPE_JOYSTICK_AXIS);
vertical_js->set_joystick_axis(JOYSTICK_AXIS_YL);

// A button as Fire1
auto fire_js = input->new_axis("Fire1");
fire_js->set_type(AXIS_TYPE_JOYSTICK_BUTTON);
fire_js->set_positive_joystick_button(JOYSTICK_BUTTON_A);
```

### Mouse Axes

Mouse movement can be captured as analog axes. This is useful for camera control or custom cursor movement.

```cpp
auto mouse_x = input->new_axis("MouseX");
mouse_x->set_type(AXIS_TYPE_MOUSE_AXIS);
mouse_x->set_mouse_axis(MOUSE_AXIS_X);

auto mouse_y = input->new_axis("MouseY");
mouse_y->set_type(AXIS_TYPE_MOUSE_AXIS);
mouse_y->set_mouse_axis(MOUSE_AXIS_Y);
```

Mouse axis values represent the **delta** in pixels that the cursor moved since the previous frame, rather than a normalised `-1.0` to `1.0` range.

### Hat (D-Pad) Axes

Gamepad D-pad hat inputs can also be mapped to axes:

```cpp
auto dpad_horizontal = input->new_axis("Horizontal");
dpad_horizontal->set_type(AXIS_TYPE_JOYSTICK_HAT);
dpad_horizontal->set_joystick_hat_axis(0, JOYSTICK_HAT_AXIS_X);
```

## Reading Axis Values

Once axes are defined, you read their values by name through the `InputManager`:

```cpp
float h = input->axis_value("Horizontal");
float v = input->axis_value("Vertical");

// Apply to movement
actor->translate(h * speed * dt, 0, v * speed * dt);
```

The returned value is the strongest (highest absolute) value among all axes sharing that name. Values are typically in the range `-1.0` to `1.0`, except for mouse movement axes which return pixel deltas.

### Hard Values

For cases where you need a discrete `-1`, `0`, or `+1` value (e.g. menu navigation), use `axis_value_hard()`:

```cpp
int8_t direction = input->axis_value_hard("Horizontal");
// Returns -1, 0, or +1
```

This rounds the axis value with the dead zone applied, returning `0` for anything within the dead zone.

## Detecting Presses and Releases

To detect when an axis transitions from inactive to active (or vice versa) use `axis_was_pressed()` and `axis_was_released()`:

```cpp
if (input->axis_was_pressed("Fire1")) {
    // Fire weapon
}

if (input->axis_was_released("Fire1")) {
    // Stop charging
}
```

These methods operate on the axis **name** and will fire true if **any** keyboard key, mouse button, or gamepad button that drives that axis was pressed or released this frame.

## Digital Input Behaviour: Force and Return Speed

When an axis is driven by a digital input (keyboard key, mouse button, gamepad button), the value does not jump instantly from `0` to `1`. Instead it ramps up and down over time, which provides smoother-feeling controls.

### Force

The `force` property controls how quickly the axis value increases while a digital input is held. It is measured in **units per second**. The default is `3.0`, meaning it takes approximately 1/3 of a second to reach `1.0`.

```cpp
auto fire = input->new_axis("Fire1");
fire->set_positive_keyboard_key(KEYBOARD_CODE_SPACE);
fire->set_force(6.0f); // Reaches 1.0 in ~1/6 of a second (snappier)
```

### Return Speed

The `return_speed` property controls how quickly the axis value decays back to `0` after the input is released. The default is also `3.0`.

```cpp
fire->set_return_speed(1.5f); // Slower release
```

Setting force and return speed lets you tune the "feel" of digital inputs to be snappy or gradual depending on your game's needs.

## Dead Zones

Analog inputs like gamepad thumbsticks produce small non-zero values even when the player is not touching them. To handle this jitter, each `InputAxis` has a `dead_zone` property.

The `dead_zone` is a value between `0.0` and `1.0`. Inputs with an absolute value less than the dead zone are treated as zero. The default is `0.001f`.

```cpp
auto stick = input->new_axis("Horizontal");
stick->set_type(AXIS_TYPE_JOYSTICK_AXIS);
stick->set_joystick_axis(JOYSTICK_AXIS_XL);
stick->set_dead_zone(0.15f); // 15% dead zone
```

### Dead Zone Behaviour

When retrieving an axis value you can choose how the dead zone is applied:

```cpp
InputAxis* axis = input->new_axis("Test");
axis->set_joystick_axis(JOYSTICK_AXIS_XL);
axis->set_dead_zone(0.1f);

// RADIAL (default) - takes the combined X/Y magnitude into account.
// Best for thumbsticks where you want a circular dead zone.
float radial = axis->value(DEAD_ZONE_BEHAVIOUR_RADIAL);

// AXIAL - applies the dead zone independently per axis.
float axial = axis->value(DEAD_ZONE_BEHAVIOUR_AXIAL);

// NONE - no dead zone filtering at all.
float raw = axis->value(DEAD_ZONE_BEHAVIOUR_NONE);
```

For joystick axes the default behaviour is `DEAD_ZONE_BEHAVIOUR_RADIAL`. This means that if the stick is pushed diagonally just outside the dead zone circle, both axes will report a value. This produces more natural-feeling movement than an axial dead zone.

Radial dead zones require access to the linked axis (the X/Y counterpart). Simulant automatically determines the linked axis for common gamepad layouts so this works transparently.

## Axis Inversion

You can invert an axis so that positive and negative inputs are swapped:

```cpp
auto inverted_vertical = input->new_axis("CameraY");
inverted_vertical->set_type(AXIS_TYPE_JOYSTICK_AXIS);
inverted_vertical->set_joystick_axis(JOYSTICK_AXIS_YR);
inverted_vertical->set_inversed(true); // Flipped for "inverted Y" camera
```

## Source Scoping

By default, axes listen to **all** devices of their type (all keyboards, all mice, all game controllers). You can scope an axis to a specific device:

```cpp
// Listen only to keyboard ID 0
axis->set_keyboard_source(KeyboardID(0));

// Listen only to mouse ID 0
axis->set_mouse_source(MouseID(0));

// Listen only to game controller index 0
axis->set_joystick_source(GameControllerIndex(0));
```

The constants `ALL_KEYBOARDS`, `ALL_MICE`, and `ALL_GAME_CONTROLLERS` (all with an internal value of `-1`) represent the default "listen to all" behaviour.

## Iterating and Destroying Axes

You can enumerate all axes or retrieve all axes with a given name:

```cpp
// Get all axes named "Horizontal"
AxisList h_axes = input->axises("Horizontal");

// Iterate every defined axis
input->each_axis([](InputAxis* axis) {
    // inspect axis
});

// How many axes share this name?
std::size_t count = input->axis_count("Fire1");
```

To destroy axes:

```cpp
// Destroy all axes with a given name
input->destroy_axises("Horizontal");

// Destroy a specific axis
input->destroy_axis(specific_axis);
```

## Pre-defined Axes

The `InputManager` creates a default set of axes on construction. These cover common needs:

| Name | Keyboard | Gamepad | Mouse |
|------|----------|---------|-------|
| `Horizontal` | A/D | Left stick X | - |
| `Vertical` | W/S | Left stick Y | - |
| `Fire1` | Space | Button A (0) | Left button (0) |
| `Fire2` | Left Ctrl | Button B (1) | Right button (1) |
| `Start` | Enter | Start button | - |
| `MouseX` | - | - | X delta |
| `MouseY` | - | - | Y delta |

You can extend or override these by creating additional axes with the same names.
