# Supported Mesh Formats

The following file formats are supported by default in Simulant

 - Wavefront OBJ
 - glTF 2.0 - Khronos Group glTF ASCII and Binary Formats (Skeletal Animation Support)
 - MS3D - Milkshape3D Animated (Skeletal Animation)
 - MD2 - Quake 2 Animated Models
 - BSP v38 - Quake 2 Level Format
 - OPT - XWing OPT Model Format
 - TMX - Tiled Map Format

# BSP v38

The Quake 2 BSP file format is supported for mesh loading in Simulant. Much of the data in the BSP file is ignored, but the following is processed:

 - Mesh data
 - Textures + Materials (limited)
 - Entities (limited, they are stashed in the mesh's data)
 - Lightmaps (Currently not enabled due to a bug)
  
# MS3D

.ms3d mesh files are fully supported. All versions of the format should be supported.

Like glTF, MS3D files are loaded as prefabs. The prefab contains an `Actor` holding the
mesh, plus a `Stage` node per MS3D joint, arranged in the same hierarchy as the file's
skeleton. The mesh itself is skinned against those joint nodes.

An MS3D file holds a single, unnamed timeline of keyframes. If the file contains any, the
prefab exposes it as an animation called `default`:

```
auto tree = scene->load_tree("models/character.ms3d");
tree->find_mixin<AnimationController>()->play("default", ANIMATION_LOOP_FOREVER);
```

# glTF

.glTF 2.0 files are supported, so long they are exported as Y+ UP. They are created as prefabs inside the engine.
When animated, the prefab spawns an AnimationController, supporting joint-based as well as skinned animations.

Please note that IK constraints are not supported at this time, and must be baked into the keyframes on export to
render properly.



