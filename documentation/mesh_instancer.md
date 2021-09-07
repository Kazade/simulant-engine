# Mesh Instancer

There are a few ways to get a `Mesh` displayed in a scene. The primary method is to create an `Actor` with a mesh, which gives you full functionality to manipulate your mesh. `Geoms` are the opposite end of the scale. They let you statically insert your `Mesh` into the scene in a very performant way, but they cannot be moved or manipulated.

However, sometimes you need some middle-ground, and that's where `MeshInstancer` comes in. A `MeshInstancer` is a stage node that itself can be moved and manipulated, but allows you to instantiate a mesh multiple times in a way that's fast, and uses much less memory than individual `Actors`. There are drawbacks though:

 - `MeshInstances` can't be moved after instantiation, although they can be removed, or have their visibility toggled.
 - Individual `MeshInstances` aren't part of the `Stage` hierarchy.
 - `MeshInstances` can't have behaviours (although the parent `MeshInstancer` can).
 - `MeshInstancer` doesn't currently support levels of detail like `Actors` do.
 - `MeshInstances` currently don't support mesh animations.
 - `MeshInstances` don't currently support material slots.

`MeshInstancers` are great for things like foliage, trees, or rocks - where you want to instantiate the same mesh many times.