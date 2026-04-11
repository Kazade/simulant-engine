# Rumble / Haptic Feedback

Simulant provides rumble (haptic feedback) support for game controllers. Rumble effects can add tactile feedback for events like weapon fire, collisions, or pickups.

## Checking for Rumble Support

Not all controllers support rumble. You can check whether a controller has rumble capability before attempting to trigger it:

```cpp
auto controller = input_state->game_controller(0);
if (controller->has_rumble_effect()) {
    // Controller supports rumble
}
```

You can also check the `has_rumble` field of the `GameControllerInfo` struct when enumerating connected controllers.

## Starting a Rumble Effect

To trigger a rumble effect, call `start_rumble()` on a `GameController` instance:

```cpp
auto controller = input_state->game_controller(0);
controller->start_rumble(0.5f, 0.8f, smlt::Seconds(1.0f));
```

The `start_rumble()` method takes three arguments:

| Parameter | Type | Description |
|-----------|------|-------------|
| `low_rumble` | `float` | Intensity of the low-frequency motor (0.0f to 1.0f) |
| `high_rumble` | `float` | Intensity of the high-frequency motor (0.0f to 1.0f) |
| `duration` | `smlt::Seconds` | How long the rumble should play |

The intensity values are normalised, where `0.0f` is no rumble and `1.0f` is maximum intensity.

### Low vs High Frequency Motors

Most modern gamepads have two rumble motors:

- **Low-frequency motor** - Produces a deep, heavy vibration. Good for impacts, explosions, and heavy impacts.
- **High-frequency motor** - Produces a sharper, lighter vibration. Good for hits, scrapes, and subtle feedback.

By mixing the two you can create a wide range of tactile effects. For example:

```cpp
// Heavy explosion - mostly low rumble
controller->start_rumble(1.0f, 0.2f, smlt::Seconds(0.5f));

// Sharp hit - mostly high rumble
controller->start_rumble(0.1f, 0.9f, smlt::Seconds(0.15f));

// Continuous engine rumble - balanced
controller->start_rumble(0.4f, 0.4f, smlt::Seconds(2.0f));
```

## Stopping Rumble

To stop any active rumble effect before it finishes:

```cpp
controller->stop_rumble();
```

This immediately halts all rumble motors on the controller.

## Platform Notes

### Dreamcast

On the Dreamcast platform only the `low_rumble` parameter has an effect. The high-frequency motor is not supported.

### General

Rumble support and behaviour may vary across platforms and controller types. Always check `has_rumble_effect()` before starting a rumble to avoid silent failures.

## Example: Rumble on Collision

Here is a simple example of integrating rumble into a collision handler:

```cpp
void on_collision(StageNode* self, StageNode* other) {
    auto controller = input_state->game_controller(0);
    if (controller && controller->has_rumble_effect()) {
        // Short sharp rumble for a hit
        controller->start_rumble(0.6f, 0.9f, smlt::Seconds(0.2f));
    }
}
```

## Example: Rumble Duration Management

If you need a rumble effect that persists for an unknown duration (e.g. while the player is charging a weapon), you can re-trigger the rumble each frame:

```cpp
void update(float dt) {
    auto controller = input_state->game_controller(0);
    if (controller && controller->has_rumble_effect() && is_charging_) {
        // Keep re-triggering a short rumble while charging
        controller->start_rumble(charge_level_, charge_level_ * 0.5f, smlt::Seconds(0.1f));
    }
}
```

When the charging is complete, call `stop_rumble()` to end the effect cleanly.
