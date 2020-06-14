# Supported Mesh Formats

The following file formats are supported by default in Simulant

 - Wavefront OBJ
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

.ms3d mesh files are fully supported. All versions of the format should be supported
and loading an MS3D file should generate a mesh skeleton, and set the correct frame rate.

Animations need to be specified by keyframe before they can be played.



