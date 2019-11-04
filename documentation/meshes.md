# Meshes

Meshes are the visible representation of everything you see in a scene (with the exception of particle systems). Meshes are essentially made up of a set of vertex data, and then multiple submeshes which store index data into the vertex set.

## Mesh::adjacency_info

For various applications it's necessary to know which polygon edges are shared with other polygons, specifically this is used when calculating shadow volumes for stencil shadowing. By default all meshes maintain adjacency info unless disabled with `Mesh::set_maintain_adjacency(false)`.

## Submeshes

`Meshes` are made up of `Submeshes`. Conceptually, each `Submesh` has its own material as well as the indices that make up the submesh (these are indices into the Mesh-level vertex data).

### Material Slots

It's quite common to allow for the same mesh to be rendered with different materials, although duplicating the mesh is an option, doing so is not very memory efficient. 

To solve this problem, each `Submesh` has 8 material slots which can be selected by setting the active material slot on the `Actor`. By default, everything uses `MATERIAL_SLOT0`. Here's an example:

```
auto mesh = stage->assets->new_mesh_from_file("mymesh.obj"); // Assume this populates slot 0

// Change the material at slot 1 of the submesh
mesh->submesh("first")->set_material_at_slot(MATERIAL_SLOT1, other_material);

auto actor1 = stage->new_actor_with_mesh(mesh); // Uses the default material at slot 0

auto actor2 = stage->new_actor_with_mesh(mesh);
actor2->use_material_slot(MATERIAL_SLOT1);  // actor2 now uses the other material
```
