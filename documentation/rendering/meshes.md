# Meshes

Meshes are the visible representation of everything you see in a scene (with the exception of particle systems). Meshes are essentially made up of a set of vertex data, and then multiple submeshes which store index data into the vertex set.

## Loading Meshes

The most common way to create a mesh is to load it from a supported file format. You can do this by calling `load_mesh_from_file(...)` on an `AssetManager`. You can optionally customise the loading of the file in a number of ways. Firstly you can provide a `VertexSpecification` for the loaded mesh to use, which allows you to customise the format of the vertex attributes.

Secondly, you can pass a `MeshLoadOptions` instance which provides a mechanism to specify additional options:

 - `cull_mode`- this sets the default face culling mode for all materials created while loading the mesh
 - `blending_enabled` - this defaults to true, but if set to false then all materials loaded will have blending forcibly disabled
 - `override_texture_extension` - This is useful if your mesh format specifies paths to textures, but you want to load a different format at runtime (e.g. a compressed texture)

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

## Skinned meshes

A mesh loaded from a format that supports skeletal animation (e.g. `.gltf`, `.ms3d`) is
*skinned*: `Mesh::is_skinned` is true and `Mesh::skin` holds the data needed to deform it.

There is no separate skeleton type on the mesh. The joints are ordinary `StageNode`s in
the scene tree, created when the prefab the mesh came from is instantiated, and the skin
simply refers to them:

 - `skin->node_indices` - the joint node for each joint index, in the order the mesh's
   vertex joint attribute refers to them
 - `skin->inverse_bind_matrices` - the inverse of each joint's bind-pose transform
 - `skin->bound_actor` - the `Actor` the skinned mesh is attached to

Each vertex carries up to four joint indices and weights (the `joint` and `weight` vertex
attributes). `Mesh::update_skinning()` blends the joint transforms per-vertex and writes
the result back into the mesh's vertex data - the `AnimationController` calls it for you
each time it applies a frame.

Because the joints are just scene nodes, posing a character is done through the normal
transform API:

```
auto neck = actor->find_descendent_with_name("neck");
neck->transform->set_rotation(my_rotation);
```

See [Skeleton Animation](../animation/skeleton-animation.md) for the full picture.

# Heightmaps

Simulant has built-in support for building heightmap terrain meshes from textures. You can create a heightmap terrain using the `new_mesh_from_heightmap` method on the `AssetManager`.

When a `Mesh` has been created from a heightmap, the mesh gains a `TerrainData` attribute in its `data`:

```
        auto data = mesh->data->get<TerrainData>("terrain_data");
```

This struct provides information about how the heightmap was generated, for example its grid spacing, min/max heights etc. `TerrainData` also provides some utility functions:

 - `TerrainData::height_at_xz(float x, float z)` - Returns the Y coordinate at the position specified. Returns an `optional<float>` which will be "falsey" if the coordinate was outside the terrain
 - `TerrainData::triangle_at_xz(float x, float z)` - Returns the 3 indexes of the triangle at the given coordinate. Returns an optional<TerrainTriangle> which will be "falsey" if the coordinate was outside the terrain
 
