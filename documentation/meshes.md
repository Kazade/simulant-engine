# Meshes

Meshes are the visible representation of everything you see in a scene (with the exception of particle systems). Meshes are essentially made up of a set of vertex data, and then multiple submeshes which store index data into the vertex set.

## Mesh::adjacency_info

For various applications it's necessary to know which polygon edges are shared with other polygons, specifically this is used when calculating shadow volumes for stencil shadowing. By default all meshes maintain adjacency info unless disabled with `Mesh::set_maintain_adjacency(false)`.

## Submeshes

`Meshes` are made up of `Submeshes`. Conceptually, each `Submesh` has its own material as well as the indices that make up the submesh (these are indices into the Mesh-level vertex data).

When creating a submesh you can optionally pass in an `IndexDataPtr` or you can just pass the `IndexType` and an `IndexData` instance will be created for you. It's sometimes useful to share index data across submeshes
for memory reasons. 

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

## Skeletons

A mesh may optional have a skeleton assigned to it - this is normally created implicitly
by loading a mesh file from a format that supports skeletal animation (e.g. .ms3d).

A skeleton contains a number of joints, and these joints are linked together to form
bones. A joint has both a relative rotation + translation (to the parent) and an absolute
rotation + translation. 

You can find and manipulate a joint by it's name:

```
Joint* joint = mesh->skeleton->find_joint("neck");
neck->rotate_to(my_rotation);
neck->move_to(my_translation);
```

Finally, skeletons maintain a relationship from vertices to joints. You can link
a vertex index to a joint by calling:

```
mesh->skeleton->attach_vertex_to_joint(joint, vertex_index, weight);
```

The `vertex_index` should be an index into the mesh's `vertex_data` array. `weight`
should be a value between 0.0 and 1.0. The total weights for a vertex should add up
to 1.0.

# Heightmaps

Simulant has built-in support for building heightmap terrain meshes from textures. You can create a heightmap terrain using the `new_mesh_from_heightmap` method on the `AssetManager`.

When a `Mesh` has been created from a heightmap, the mesh gains a `TerrainData` attribute in its `data`:

```
        auto data = mesh->data->get<TerrainData>("terrain_data");
```

This struct provides information about how the heightmap was generated, for example its grid spacing, min/max heights etc. `TerrainData` also provides some utility functions:

 - `TerrainData::height_at_xz(float x, float z)` - Returns the Y coordinate at the position specified. Returns an `optional<float>` which will be "falsey" if the coordinate was outside the terrain
 - `TerrainData::triangle_at_xz(float x, float z)` - Returns the 3 indexes of the triangle at the given coordinate. Returns an optional<TerrainTriangle> which will be "falsey" if the coordinate was outside the terrain
