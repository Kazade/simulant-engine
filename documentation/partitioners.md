
# Partitioners

During the render process it is important for the engine to be able to quickly gather the StageNodes that are visible for rendering. The best method for quickly calculating
this visible geometry may vary depending on the type of scene which is being rendered. For example, a 3D terrain may be better suited to some kind of Quadree based culling while an
enclosed level may be rendered more efficiently using some kind of BSP-type culling.

Simulant uses a simple pluggable system for defining _partitioners_ that provide two methods to the renderinga  system:

 - `geometry_visible_from(CameraID)`
 - `lights_visible_from(CameraID)`

The partitioner keeps track of all `StageNodes` (aside from the `Stage` itself) and then returns the visible geometry or lights when required. The partitioner is selected when
a `Stage` is constructed (via `new_stage()`).

One thing to note is that if a `Geom` is visible, the renderable returned will be the result of an additional Octree culling step. This allows for entire chunks of static
geometry to be culled out by the partitioner, and then if they are visible, to only return the polygons visible rather than the whole chunk.

## Geom Octree Culling

When you load a mesh, there are two ways of rendering it. You can either attach it to an `Actor`; allowing
you to move and rotate it. Or, you can generate a `Geom` from it. `Geoms` are static once they have been
created and they internally use an Octree to provide an additional culling step once the partitioner has
deemed it visible.

When creating a Geom, as well as specifying its rotation and position, you can also specify the depth
of the octree. This is really a per-mesh setting that you'll need to test and profile. If the max depth
is too high, then the processing time will outweigh any savings on overdraw, if it's too low you'll still
be drawing too many polygons outside the view frustum.

**NOTE: Once you have created a `Geom` from a `MeshPtr`, DO NOT manipulate the `Mesh's` `vertex_data`. Doing so will
cause visual corruption or a crash! You might be able to get away with manipulating diffuse colours, texture coordinates
or normals, but changing the positions or number of vertices will cause errors.**
