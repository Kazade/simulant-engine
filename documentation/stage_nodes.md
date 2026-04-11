# Stage Nodes

Each `Stage` is the root of a heirachy of `StageNodes`. Some classes that inherit `StageNode` are:

 - `Stage`
 - `Actor`
 - `ParticleSystem`
 - `Geom`
 - `Camera`
 - `Light`
 
These are the basic building blocks of your scene.

## Building a Heirarchy

`StageNodes` can be parents and children of other `StageNodes`. To build this heirarchy you make use of the `StageNode::set_parent` method. `set_parent` can take a node ID, or the node itself.

All `StageNodes` by default are children of the `Stage` they belong to. If you want to detach a node from its parent (and reparent it under the `Stage`) then simply pass a `nullptr` to `set_parent`:

```
auto a1 = create_child<Actor>();
auto a2 = create_child<Actor>();
a2->set_parent(a1);

// a1 is now a2's parent

a2->set_parent(nullptr);

// a1 and a2 are now both direct children of stage_
```

## Traversal

There are a number of ways to traverse your stage heirarchy but they are all exposed through
standard C++ begin/end pairs designed to be used with C++ range for-loops. The below code
shows a number of examples of traversing a stage's tree:

```
for(auto node: stage->each_descendent()) {}  // Root-to-leaf iteration

for(auto node: actor->each_ancestor()) {} // Traverse up the tree

for(auto node: actor->each_sibling()) {} // Iterate the siblings of the actor

for(auto node: stage->each_child()) {} // Iterate direct-children only
```

# Updating of StageNode and their Behaviours

`StageNodes` are only updated if the owning `Stage` is attached to an active `Layer`. You can check this by checking `Stage::is_part_of_active_pipeline()`. 


# Destruction

You can destroy a `StageNode` by calling its `destroy()` method. This won't release the `StageNode` immediately, but it will fire the `signal_destroyed` signal. The actual clean-up of the node will happen after `late_update()`, but before the render queue is built. This helps prevent issues where queued tasks try to access deleted nodes.

When the clean-up process runs, an additional `signal_cleaned_up` signal will fire just before deletion of the node.

## Delayed Destruction

`StageNodes` have a helper method called `destroy_after(Seconds)` that you can call to fire `destroy()` after the number of seconds have passed. Internally this simply queues a coroutine to call `destroy()`. Be aware, `destroy_after` is a fire-and-forget method - you can't cancel once you've triggered it! 

 > Note: `is_marked_for_destruction()` will return false until the elapsed time has passed, and `destroy()` has been called.

## Finding nodes

You can give stage nodes names using `StageNode::set_name`, and later search for those nodes recursively using `StageNode::find_descendent`. If a name is duplicated, only the first match found will be returned.

You can set a name during construction of a stage node by chaining using the `set_name_and_get(name)` method:

```
auto actor = create_child<Actor>()->set_name_and_get("Actor 1");
```

# Disabling Culling

By default, stage nodes (excluding cameras) will be culled with a scene partitioner if they are deemed to be offscreen. You can disable this behaviour for a stage node by toggling its cullable state:

```
node->set_cullable(false);  // Will always be renderered
assert(!node->is_cullable());
```

# Mixins

StageNodes normally follow a parent-child hierarchy. Sometimes this needlessly complicates your scene graph when you want to combine the behaviour of multiple nodes. If a StageNode is well designed it can be used as a "mixin" with another node. For example:

```
auto node = create_child<Actor>();
auto mixin = node->create_mixin<MyStageNode>();
```

In the above case, the Actor will be the node that is part of the scene graph, but both Actor and MyStageNode will share a `Transform` and both will have their `on_update` methods called. This is a very powerful feature that allows you to build complex nodes without cluttering up your scene tree.

## Finding Dependent Nodes

Often when writing a `StageNode` you'll need access to other nodes that are related to the one you are defining the behaviour for. For example, if you are writing a vehicle node, you may need to access the wheel stage nodes which are children of the main car body node. You can use "finders" for this purpose:

```
class CarBehaviour : public StageNode {
public:
    FindResult<Actor> front_left_wheel = FindDescendent("Front Left", this);  // passing `this` is an unfortunate necessity to allow this syntax to work
    FindResult<Actor> front_right_wheel = FindDescendent("Front Right", this);
}
```

These variables give you quick access to those child nodes. Likewise, `FindAncestor` will search up the stage node tree for a matching parent node. 

When you access `front_left_wheel` above, a search will happen down the tree for the node named "Front Left" and the result will be cached for future accesses. If the found node is later destroyed, the cache will be invalidated and the next access will search again.

## Rigid Body Physics

Simulant's rigid body simulation is implemented entirely using `Behaviours`. The key `Behaviours` to examine are:

 - `RigidBody` - a dynamic physics object
 - `StaticBody` - a static physics object
 - `KinematicBody` - a kinematic physics object
 - `RaycastVehicle` - a work-in-progress `Behaviour` for non-realistic car physics

All of these `Behaviours` require a `RigidBodySimulation` instance to function. The easiest way to get access to one of these is to make use of the `PhysicsScene` class when constructing your game scene.
