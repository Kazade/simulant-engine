# Actors

Actors are the predominant object you will manipulate to construct your scene. They represent a
movable object within the `Stage`.

An `Actor` is a `StageNode` and is usually linked to a `Mesh` for rendering. As they are `StageNode`s, Actors can be attached to other `StageNode`s in parent-child relationships and moved around the scene.

## Associating a Mesh

When you construct an Actor (without passing a mesh ID) it is empty, and simply represents an element in the `Stage`s node tree. 

To make the Actor visible you need to attach a mesh to it. You can do that with the `set_mesh(MeshID, DetailLevel)` method.

Once a mesh is attached you will see it rendered at the Actor's location.

## Detail Meshes

An actor can have up to 5 meshes attached at different detail levels. When rendering
one of these meshes will be selected depending on the Actor's distance from the Camera.

The "base" mesh is the one registered with a detail level of `DETAIL_LEVEL_NEAREST`. This
base mesh is special in a number of ways:

 - Only the nearest mesh can be animated.
 - The AABB of the Actor is determined from the nearest mesh.
 - You cannot attach a mesh at any other level unless you have specified a base mesh.
 
You can access the base mesh through the `base_mesh()` method, or alternatively by passing
`DETAIL_LEVEL_NEAREST` to the `mesh(DetailLevel)` method.

## Detail Mesh Selection

Simulant will determine the ideal `DetailLevel` using the ranges defined on the `Pipeline` being rendered. If there is no mesh attached to the Actor at the ideal level, then the next nearest
available mesh will be rendered instead.

To define the distance ranges that activate each detail level you must use the `Pipeline::set_detail_level_distances(nearest_cutoff, near_cutoff, mid_cutoff, far_cutoff)` method. For example:

```
auto pipeline = compositor->render(stage, camera);
pipeline->set_detail_level_distances(10.0f, 20.0f, 40.0f, 80.0f);
```

Any distance above `far_cutoff` will use the `DETAIL_LEVEL_FARTHEST` level. Any below
`nearest_cutoff` will use the `DETAIL_LEVEL_NEAREST` level.

# Skinned actors

When an `Actor`'s base mesh is skinned (`mesh->is_skinned`), the joints that deform it are
ordinary `StageNode`s alongside the `Actor` in the prefab it was instantiated from, rather
than a separate rig object owned by the `Actor`.

That means posing a character is just moving scene nodes. For example, to make an animated
character's head look in a particular direction, create a mixin with a `late_update()`
method - which runs after the `AnimationController` has applied the current frame - and do
something similar to the following:

```
Quaternion rotation = calculate_rotation_to_look();
find_descendent_with_name("Neck")->transform->set_rotation(rotation);
```

This allows you to take manual control of joints, even if the rest of the mesh is animated.
The skinning itself is recalculated by `Mesh::update_skinning()`.

