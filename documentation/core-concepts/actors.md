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

`Actor` renders a mesh as-is, so it is *not* what renders a skinned character. Meshes hold
source vertex data only and are shared between every node that uses them, which means
posing one in place would corrupt every other user of it.

Skinned meshes are rendered by an `Armature` instead. An `Armature` is the root of a
skeleton of `Joint` nodes, owns a private posed copy of each skinned mesh bound to it, and
submits that copy to the render queue. See
[Skeleton Animation](../animation/skeleton-animation.md).

An `Actor` is still the right thing for anything *attached* to a skeleton but not deformed
by it - a sword, a hat, a backpack. Because joints are ordinary scene nodes, that's just
parenting:

```
auto sword = create_child<Actor>(assets->load_mesh("models/sword.obj"));
sword->set_parent(armature->find_joint("hand.r"));
```
