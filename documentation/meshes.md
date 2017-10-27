# Meshes

Meshes are the visible representation of everything you see in a scene (with the exception of particle systems). Meshes are essentially made up of a set of vertex data, and then multiple submeshes which store index data into the vertex set.

## Mesh::adjacency_info

For various applications it's necessary to know which polygon edges are shared with other polygons, specifically this is used when calculating shadow volumes for stencil shadowing. By default all meshes maintain adjacency info unless disabled with `Mesh::set_maintain_adjacency(false)`.


