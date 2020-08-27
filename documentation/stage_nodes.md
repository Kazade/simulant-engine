# Stage Nodes

Each `Stage` is the root of a heirachy of `StageNodes`. Classes that inherit `StageNode` are:

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
auto a1 = stage_->new_actor();
auto a2 = stage_->new_actor();
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

`StageNodes` are only updated if the owning `Stage` is attached to an active `Pipeline`. You can check this by checking `Stage::is_part_of_active_pipeline()`. 

It is highly recommended that if you are manipulating a `Stage` in a background thread (e.g. in the `load()` method of a `Scene`) that you do not attach the `Stage` to a pipeline until you are finished manipulating. This should prevent threading problems.

# Destruction

You can destroy a `StageNode` by calling its `destroy()` method. This won't release the `StageNode` immediately, but it will fire the `signal_destroyed` signal. The actual clean-up of the node will happen when any idle tasks have run, but before the render queue is built. This helps prevent issues where queued tasks try to access deleted nodes.

When the clean-up process runs, an additional `signal_cleaned_up` signal will fire just before deletion of the node.

## Delayed Destruction

`StageNodes` have a helper method called `destroy_after(Seconds)` that you can call to fire `destroy()` after the number of seconds have passed. Internally this simply queues an idle task to call `destroy()`. Be aware, `destroy_after` is a fire-and-forget method - you can't cancel once you've triggered it! 

 > Note: `is_marked_for_destruction()` will return false until the elapsed time has passed, and `destroy()` has been called.

## Finding nodes

You can give stage nodes names using `StageNode::set_name`, and later search for those nodes recursively using `StageNode::find_descendent_with_name`. If a name is duplicated, only the first match found
will be returned.

You can set a name during construction of a stage node by chaining using the `set_name_and_get(name)` method:

```
auto actor = stage->new_actor()->set_name_and_get("Actor 1");
```

# Disabling Culling

By default, stage nodes (excluding cameras) will be culled with a scene partitioner if they are deemed to be offscreen. You can disable this behaviour for a stage node by toggling its cullable state:

```
node->set_cullable(false);  // Will always be renderered
assert(!node->is_cullable());
```
