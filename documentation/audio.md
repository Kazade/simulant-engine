# Audio

Simulant has a full-featured audio system that makes it easy to load and play sound effects and streams.

## Loading Sounds

`Sounds` are an `Asset`, like `Textures` or `Materials`, and so can be loaded using an asset manager, probably using
`new_sound_from_file(path);`

## Playing Sounds

`Sounds` are played through `Sources`. All `StageNodes` are a sound `Source`, as is the `Window` itself. You can play a sounds
by using the `play_sound(id, repeat)` method on the `Source`. In the case of the `Window` the `id` must be the ID of a sound
loaded by the `shared_assets` manager, in the case of `StageNodes` the sound must be either in the associated `Stage` asset manager
or `shared_assets`.

Playing a sound from a `StageNode` will use positional audio to make it seem like the sound is coming from the location of the node. Sounds played
through the `Window` will not have any positional effect - this makes the `Window` perfect for playing a level soundtrack for example.

## Controlling the volume

The volume of a playing sound can be controlled on a per-Source basis using `Source::set_gain(gain)` where gain is clamped between 0 and 1. By default
all sounds are played at maximum gain.

## Playing sounds beyond the lifetime of a node

Sometimes you will have a situation where you need to play a sound from the position of a `StageNode`, but the node itself is due to be destroyed. For example,
an explosion sound effect might outlive the `ParticleSystem` that represents the explosion - so how can you play the full sound?

There are a few options, here are a couple. 

One option is to hide the node, but destroy it later:

```
node->set_visible(false);
node->play_sound(explosion);
node->destroy_after(Seconds(5));
```

Alternatively, you could specifically create a node to play the sound effect:

```
auto sound_node = node->stage->new_actor();
sound_node->move_to(node->absolute_position());
sound_node->play_sound(explosion);
sound_node->destroy_after(Seconds(5));
```

