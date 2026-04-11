# Positional Audio and 3D Sound

Simulant provides built-in support for positional (3D) audio, allowing sounds to be placed in three-dimensional space so they appear to originate from specific locations in your game world.

## How Positional Audio Works

When a sound is played through an `AudioSource` (which all `StageNodes` are), Simulant uses the position and orientation of both the sound source and the audio listener to calculate how the sound should be perceived. This includes:

- **Distance attenuation** - Sounds get quieter as they move further from the listener
- **Stereo panning** - Sounds are panned left or right based on their position relative to the listener

All `StageNodes` are `AudioSource` instances, so any node in your scene can emit sound from its position:

```cpp
auto enemy = scene->create_child<smlt::StageNode>();
enemy->move_to(smlt::Vec3(5.0f, 0.0f, 3.0f));
enemy->play_sound(explosion_sound);
```

The sound will appear to come from the enemy's position in 3D space.

## Distance Models

Simulant supports two distance models that control how sounds are spatialized:

### DISTANCE_MODEL_POSITIONAL

This is the default model. Sounds are treated as originating from a specific point in space. Volume decreases with distance from the listener, and the sound is panned based on its position relative to the listener's orientation.

```cpp
// Explicitly use positional audio (this is the default)
node->play_sound(sound, AUDIO_REPEAT_NONE, smlt::DISTANCE_MODEL_POSITIONAL);
```

### DISTANCE_MODEL_AMBIENT

Ambient sounds ignore positional audio entirely. They are played at full volume regardless of the listener's position and orientation. This is ideal for background music, ambient noise, or UI sounds.

```cpp
// Play as ambient sound (e.g., background music)
node->play_sound(music, AUDIO_REPEAT_FOREVER, smlt::DISTANCE_MODEL_AMBIENT);
```

You will commonly see this used for soundtracks:

```cpp
auto music_node = scene->create_child<smlt::StageNode>();
music_node->play_sound(
    assets->load_sound("music/level.ogg"),
    smlt::AUDIO_REPEAT_FOREVER,
    smlt::DISTANCE_MODEL_AMBIENT
);
```

## The Audio Listener

Positional audio requires a listener - a point in space from which sounds are heard. Simulant applications have a global audio listener stored on the `Window`.

### Default Listener

By default, the audio listener is automatically set to the `Camera` used in the first active pipeline in the compositor. If there are no active pipelines, there is no listener and positional audio will not work correctly.

### Setting a Custom Listener

If you have multiple pipelines or need explicit control over the listener position, you can set it manually using `Window::set_audio_listener(node)`:

```cpp
// Set the listener to a specific node
window->set_audio_listener(player_camera);
```

The listener tracks the node's position and rotation. If the listener node is destroyed, listener behaviour reverts to the default automatic selection. You can check whether an explicit listener is set:

```cpp
if (window->has_explicit_audio_listener()) {
    S_DEBUG("Using explicit audio listener");
} else {
    S_DEBUG("Using default pipeline camera as listener");
}

auto* listener = window->audio_listener();
if (listener) {
    S_DEBUG("Listener position: {}", listener->transform->position());
}
```

### Doppler Effect

Simulant supports the Doppler effect, which shifts the pitch of sounds based on the relative velocity between the sound source and the listener. The audio system automatically calculates velocity based on how far a source has moved between frames, so moving objects will naturally produce Doppler shifts without additional configuration.

The Doppler factor and speed of sound can be configured on the `SoundDriver`:

```cpp
// Increase the Doppler effect intensity
application->sound_driver->set_doppler_factor(2.0f);

// Adjust the speed of sound (default is typically 343.3 m/s)
application->sound_driver->set_speed_of_sound(343.3f);
```

## Reference Distance

The reference distance controls the distance at which a sound is at full volume. Beyond this distance, attenuation begins. You can adjust this on a per-sound basis using `PlayingSound::set_reference_distance(dist)`:

```cpp
auto playing = node->play_sound(sound);
if (playing.is_valid()) {
    // Sound will be at full volume within 5 units
    playing->set_reference_distance(5.0f);
}
```

## Per-Sound Volume Control

Volume is controlled per `AudioSource` using `set_gain(gain)`, where gain is a `NormalizedFloat` clamped between 0 and 1:

```cpp
auto playing = node->play_sound(sound);
if (playing.is_valid()) {
    playing->set_gain(0.5f);  // 50% volume
}
```

By default, all sounds play at maximum gain (1.0).

## Pitch Control

You can also adjust the pitch of individual playing sounds:

```cpp
auto playing = node->play_sound(sound);
if (playing.is_valid()) {
    playing->set_pitch(1.5f);  // 1.5x pitch (higher)
}
```

This is useful for varying repetitive sound effects - playing the same sound at slightly different pitches can make repeated events feel less monotonous.

## Practical Examples

### Moving Sound Source

Sounds from moving objects automatically update their position each frame:

```cpp
// A car driving past the player
auto car = scene->create_child<smlt::StageNode>();
car->move_to(smlt::Vec3(-50.0f, 0.0f, 10.0f));
auto engine_sound = car->play_sound(
    assets->load_sound("sfx/car_engine.ogg"),
    smlt::AUDIO_REPEAT_FOREVER,
    smlt::DISTANCE_MODEL_POSITIONAL
);

// As the car moves, the sound position updates automatically
car->translate_by(smlt::Vec3(1.0f, 0.0f, 0.0f) * dt);
```

### Static Sound Emitter

For sounds that should come from a fixed location (like a waterfall or machinery):

```cpp
auto waterfall_node = scene->create_child<smlt::StageNode>();
waterfall_node->move_to(smlt::Vec3(20.0f, 0.0f, -15.0f));
waterfall_node->play_sound(
    assets->load_sound("sfx/waterfall.ogg"),
    smlt::AUDIO_REPEAT_FOREVER,
    smlt::DISTANCE_MODEL_POSITIONAL
);
```

### Ambient Zone with Multiple Sounds

You can create ambient zones by placing several positional sounds around an area:

```cpp
// Forest ambient
auto forest_node = scene->create_child<smlt::StageNode>();
forest_node->move_to(smlt::Vec3(0, 0, 0));
forest_node->play_sound(
    assets->load_sound("ambient/forest_wind.ogg"),
    smlt::AUDIO_REPEAT_FOREVER,
    smlt::DISTANCE_MODEL_POSITIONAL
);

// Bird sounds scattered around
for (int i = 0; i < 3; ++i) {
    auto bird = scene->create_child<smlt::StageNode>();
    bird->move_to(smlt::Vec3(
        smlt::math::random_range(-20.0f, 20.0f),
        smlt::math::random_range(5.0f, 15.0f),
        smlt::math::random_range(-20.0f, 20.0f)
    ));
    bird->play_sound(
        assets->load_sound("sfx/bird_chirp.wav"),
        smlt::AUDIO_REPEAT_NONE,
        smlt::DISTANCE_MODEL_POSITIONAL
    );
}
```

## Playing Sounds Beyond Node Lifetime

A common scenario is needing to play a sound from the position of a `StageNode` that is about to be destroyed - for example, an explosion sound that outlives the exploding object.

One approach is to hide the node and destroy it later:

```cpp
node->set_visible(false);
node->play_sound(explosion);
node->destroy_after(smlt::Seconds(5));
```

Alternatively, create a dedicated node for the sound:

```cpp
auto sound_node = node->stage->new_actor();
sound_node->move_to(node->absolute_position());
sound_node->play_sound(explosion);
sound_node->destroy_after(smlt::Seconds(5));
```

## Signals

`AudioSource` nodes emit signals you can use to react to sound events:

- `signal_sound_played(sound, repeat, model)` - Fired when a sound starts playing
- `signal_stream_finished()` - Fired when a streamed sound completes

```cpp
audio_source->signal_sound_played().connect([&](smlt::SoundPtr sound, smlt::AudioRepeat repeat, smlt::DistanceModel model) {
    S_DEBUG("Started playing sound");
});
```
