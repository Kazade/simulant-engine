
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

