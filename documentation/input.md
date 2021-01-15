
# Handling Input

There are two classes which provide access to input devices, these are the `InputManager` and `InputState`. `InputState` holds the current state of all connected keyboards, mice and joysticks; the state of their buttons and axises. `InputManager` is an abstraction above `InputState` which allows you define virtual `InputAxis` instances which reflect the state of one or more buttons or axises from the `InputState`.

## Reading Input

The Window and Scene have a property called `input`, both refer to the `InputManager` for the Window. To get the value of an axis you call the `InputManager::axis_value(name)` method. `axis_value()` returns a value between -1.0 and 1.0 (in most cases). In the case of mouse input the value will reflect the mouse-delta from the previous frame, and so will return a value in pixels that the cursor moved.

## Detecting key/button Presses or Releases

Quite often you'll need to discover whether a key was just pressed, or just released in the current frame. You can do this by using the `axis_was_pressed(name)` and `axis_was_released(name)` methods of the `InputManager`. Note, these methods operation on axis names, not on key codes, and will return true if any keyboard key, mouse button, or joystick button activates/deactivates the axis.


## Defining Axises

To define a new axis, you should use `input->new_axis(name)`. This will initially create a non-functional axis where the `type()` of the axis will be `AXIS_TYPE_UNSET`. To make the axis work, you need to set either positive or negative buttons or keys, or a physical axis (e.g joystick axis, or mouse axis).

```
auto new_axis = input->new_axis("MyAxis");
new_axis->set_positive_keyboard_key(KEYBOARD_CODE_A);
new_axis->type();  // -> AXIS_TYPE_KEYBOARD_KEY
```

You can create multiple axises with the same name, but input value with the "strongest" value will apply.

## Pre-defined Axises


## Axis forces

`InputAxis` have a `force` variable that is used when processing inputs from digitial devices like keyboard keys, or joystick buttons. The force is the speed in units-per-second that the axis value increases until it gets to +/- 1.0. You can adjust the force of an axis with the `set_force` method. This is conceptually the opposite of `return_speed`, but only applies to digital inputs.

## Axis dead zones

An `InputAxis` can be given a `dead_zone` value to help handle the problem that most joysticks have a bit of jitter when they're at rest. The `dead_zone` is a value between 0.0 and 1.0. Inputs with a value less than the dead zone are ignored by default. For joystick devices with an X and Y axis, the default behaviour is to take into account the combined input to determine whether the value falls within the dead zone. This is a "radial" dead zone as opposed to an "axial" one. You can control the dead zone behaviour when retrieving an `Axis`' value:

```
    InputAxis* axis = manager_->new_axis("Test");
    axis->set_joystick_axis(JOYSTICK_AXIS_0);
    axis->set_dead_zone(0.1f);
    
    auto radial = axis->value(/* DEAD_ZONE_BEHAVIOUR_RADIAL */); // default
    auto axial = axis->value(DEAD_ZONE_BEHAVIOUR_AXIAL);
    auto no_dead_zone = axis->value(DEAD_ZONE_BEHAVIOUR_NONE);
```
