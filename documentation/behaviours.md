# Organisms and Behaviours


> In biology, an organism is any individual entity that exhibits the properties of life. It is a synonym for "life form". - Wikipedia


Simulant `StageNodes` support pluggable controllers called `Behaviours`. Behaviours allow you to build modular logical components and then attach them to an `Organism` (an `Actor` for example). This essentially gives the `Organism` a life of its own.

## Defining a Behaviour 

Behaviours are classes that have three requirements:

 - They must derive Behaviour
 - They must derive RefCounted<T>
 - They must override the `name()` method
 
 A basic `Behaviour` looks like this:
 
 ```
 class MyBehaviour : public Behaviour, public RefCounted<MyBehaviour> {
 public:
     const char* name() const { return "My Behaviour"; }
 };
 ```

To instantiate a behaviour, you must call `new_behaviour<T>()` on the node you wish to apply the behaviour to:

```
auto new_behaviour = actor->new_behaviour<MyBehaviour>();
```

## Finding Dependent Nodes

Often when writing a `Behaviour` you'll need access to other nodes that are related to the one you are defining the behaviour for. For example, if you are writing a vehicle behaviour, you may need to access the wheel stage nodes which are children of the main car body node. You can use "finders" for this purpose:

```
class CarBehaviour : public Behaviour, public RefCounted<CarBehaviour> {
public:
    FindResult<Actor> front_left_wheel = FindDescendent("Front Left", this);  // passing `this` is an unfortunate necessity to allow this syntax to work
    FindResult<Actor> front_right_wheel = FindDescendent("Front Right", this);
}
```

These variables give you quick access to those child nodes. Likewise, `FindAncestor` will search up the stage node tree for a matching parent node.

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
 - `KinematicBody` - a kinematic physics object
 - `RaycastVehicle` - a work-in-progress `Behaviour` for non-realistic car physics

All of these `Behaviours` require a `RigidBodySimulation` instance to function. The easiest way to get access to one of these is to make use of the `PhysicsScene` class when constructing your game scene.

