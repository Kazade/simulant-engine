# Particle System File Format (.kglp)

Simulant uses a custom file format to define particle system properties. These files are JSON files with a particular set of keys and values. Files use the `.kglp` or `.spart` extension.

## System properties

The JSON file should have a root dictionary which defines the particle system, it can have the following properties:

 - `name` (string): A human readable description for the particle system. Default: `"unnamed"`.
 - `quota` (integer): The maximum number of live particles that the particle system can create across all emitters. Default: `0`.
 - `particle_width` (float): The width of the particle sprites in world units. Default: `0.0`.
 - `particle_height` (float): The height of the particle sprites in world units. Default: `0.0`.
 - `cull_each` (boolean): If true each particle will be individually culled **(not yet implemented)**. Default: `false`.
 - `emitters` (array): A list of dictionaries, each defining the properties of a particle emitter.
 - `manipulators` (array): A list of dictionaries, each defining a rule that affects particles each frame.
 - `material` (string): Either a path to a material file, or the name of a built-in material (e.g. `"TEXTURED_PARTICLE"`). If omitted, the default material is used.
 - `material.XXXXX`: These keys allow you to set individual material properties on the specified material. Their type depends on the property type of the Material property. Supported property types are `bool`, `float`, `int`, and `texture`. Texture paths are resolved relative to the particle script's directory.

## Emitter properties

 - `type` (string): The type of emitter. Valid options are `"point"` or `"box"`. Default: `"point"`.
 - `direction` (string): The (relative) direction the particles travel. A space-separated list of x, y, z floats. Default: `"0 1 0"`.
 - `velocity` (float): The speed at which particles are emitted. Sets both min and max to the same value.
 - `velocity_min` (float): The minimum emission speed. Used together with `velocity_max`.
 - `velocity_max` (float): The maximum emission speed. Used together with `velocity_min`.
 - `width` (float): The width of the emitter volume (for box emitters).
 - `height` (float): The height of the emitter volume (for box emitters).
 - `depth` (float): The depth of the emitter volume (for box emitters).
 - `ttl` (float): The time in seconds that a particle lives. Sets both min and max to the same value.
 - `ttl_min` (float): The minimum time in seconds that a particle lives.
 - `ttl_max` (float): The maximum time in seconds that a particle lives.
 - `angle` (float): The angle in degrees from the direction vector that the particle can travel. The greater the angle, the more the particle spread.
 - `color` (string): The colour of the particles emitted. Supports three formats:
   - Space-separated RGB: `"R G B"` (alpha defaults to 1.0)
   - Space-separated RGBA: `"R G B A"`
   - Hex: `"#RRGGBB"` or `"#RRGGBBAA"` (case-insensitive)
   
   Default: `"1 1 1 1"` (white, fully opaque).
 - `colors` (array of strings): An alternative to `color` that specifies multiple colours. Each element uses the same format as `color`. If both `color` and `colors` are present, `color` takes precedence.
 - `emission_rate` (float): How many particles can be emitted per second.
 - `duration` (float): How long in seconds that the emitter should last. Zero means forever.
 - `repeat_delay` (float): If set, the emitter will restart its duration after this many seconds (e.g. repeated bursts of particles).

## Manipulator properties

 - `type` (string): The type of the manipulator. Valid options are:
   `size`,
   `color_fader`,
   `alpha_fader`,
   `direction`,
   `direction_noise_random`

Additional properties depend on the type of the manipulator.

### Size

 - `rate` (float): A value between -1.0 and +1.0, the amount per second to change the size of the particle. Negative values shrink, positive values grow.
 - `curve` (string): This can be used instead of `rate` to use a curve function for the sizing. Options are `linear(rate)`, or `bell(peak, deviation)`.

### Color Fader

 - `colors` (array): A list of strings in the format `"R G B"`, `"R G B A"`, or `"#RRGGBB"` / `"#RRGGBBAA"` where each component is a floating point number between 0.0 and 1.0 (or a hex value).
 - `interpolate` (bool): Whether or not to blend between colours. Default: `true`.

### Alpha Fader

 - `alphas` (array): A list of floating point numbers between 0.0 and 1.0 representing alpha values.
 - `interpolate` (bool): Whether or not to blend between alpha values. Default: `true`.

### Direction

 - `force` (string): A vector described in the format `"X Y Z"` which defines the force per second to be applied to each particle's position.

### Direction Noise Random

 - `force` (string): A vector described in the format `"X Y Z"` which defines the force per second to be applied to each particle's position.
 - `noise_amount` (string): A vector described in the format `"X Y Z"` which defines how much noise per second is added to each particle's direction.
