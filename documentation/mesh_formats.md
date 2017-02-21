# Supported Mesh Formats

The following file formats are supported by default in Simulant

 - Wavefront OBJ
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


