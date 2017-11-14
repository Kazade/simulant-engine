# Organisms and Behaviours


> In biology, an organism is any individual entity that exhibits the properties of life. It is a synonym for "life form". - Wikipedia


Simulant `StageNodes` support pluggable controllers called `Behaviours`. Behaviours allow you to build modular logical components and then attach them to an `Organism` (an `Actor` for example). This essentially gives the `Organism` a life of its own.

## Built-in Behaviours

Simulant comes with a number of built-in `Behaviours` and more and more of these will be added over time. Some of the built-in behaviours are:

 - `HoverShip` - Makes the `Organism` hover above the ground and respond to controls.
 - `Airplane` - Very basic flight-sim style controller.
 - `SmoothFollow` - Primarily designed for `Cameras` this makes the `Organism` follow a target.

You can explore the full list of behaviours in the `simulant/behaviours` directory and subdirectories.

## Rigid Body Physics

Simulant's rigid body simulation is implemented entirely using `Behaviours`. The key `Behaviours` to examine are:

 - `RigidBody` - a dynamic physics object
 - `StaticBody` - a static physics object
 - `RaycastVehicle` - a work-in-progress `Behaviour` for non-realistic car physics

All of these `Behaviours` require a `RigidBodySimulation` instance to function. The easiest way to get access to one of these is to make use of the `PhysicsScene` class when constructing your game scene.

