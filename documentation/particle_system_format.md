# Particle System File Format (.kglp)

Simulant uses a custom file format to define particle system properties. These files are JSON files with a particular set of keys and values.

## System properties

The JSON file should have a root dictionary which defines the particle system, it can have the following properties:

 - name (string): This is a human readable description for the particle system
 - quota (integer): This is the maximum number of live particles that the particle system can create across all emitters
 - particle_width (float): This is the width of the particle sprites in world units
 - particle_height (float): This is the height of the particle sprites in world units
 - cull_each (boolean): If true each particle will be individually culled **(not yet implemented)**
 - emitters (array): A list of dictionaries, each defining the properties of a particle emitter 
 - manipulators (array): A list of dictionaries, each defining a rule that affects particles each frame
 - material (string): Either a path to a material file, or the name of a built-in material (e.g. `"TEXTURED_PARTICLE"`)
 - material.XXXXX: These keys allow you to set individual material properties on the specified material, their type depends on the property type of the Material property.

## Emitter properties

 - type (string): This is the type of emitter the valid options are "point" or "box"
 - direction (string): This is the (relative) direction the particles travel. It's a space separated list of x, y, z floats.
 - velocity (float): This is the speed at which particles are emitted
 - ttl_min (float): The minimum time in seconds that a particle lives
 - ttl_max (float): The maximum time in seconds that a particle lives
 - angle (float): The angle in degrees from the direction vector that the particle can travel. The greater the angle, the more the particle spread
 - colour (string): The colour of the particles emitted. This is a space separated list of r, g, b and alpha values (between 0.0 and 1.0)
 - emission_rate (integer): A many particles can be emitted per second
 - duration (float): How long in seconds that the emitter should last. Zero means forever
 - repeat_delay (float): If set, the emitter will restart it's duration after this many seconds (e.g. repeated bursts of particles)

## Manipulator properties 

 - type (string): The type of the affector, valid options are: `size`, `colour_fader`.
 
Additional properties depend on the type of the affector

### Size

 - rate (float): A value between -1.0 and +1.0, the amount per second to reduce the size of the particle

### Colour Fader **(Not Implemented)**

 - red (float): The amount per second added to the particle red (limited in the range 0.0 to 1.0)
 - green (float): The amount per second added to the particle green (limited in the range 0.0 to 1.0)
 - blue (float): The amount per second added to the particle blue (limited in the range 0.0 to 1.0)
 - alpha (float): The amount per second added to the particle alpha (limited in the range 0.0 to 1.0)
