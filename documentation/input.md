
# Handling Input

There are two classes which provide access to input devices, these are the `InputManager` and `InputState`. `InputState` holds the current state of all connected keyboards, mice and joysticks; the state of their buttons and axises. `InputManager` is an abstraction above `InputState` which allows you define virtual `InputAxis` instances which reflect the state of one or more buttons or axises from the `InputState`.

## Reading Input

The Window and Scene have a property called `input`, both refer to the `InputManager` for the Window. To get the value of an axis you call the `InputManager::axis_value(name)` method. `axis_value()` returns a value between -1.0 and 1.0 (in most cases). In the case of mouse input the value will reflect the mouse-delta from the previous frame, and so will return a value in pixels that the cursor moved.

## Defining Axises

## Pre-defined Axises

