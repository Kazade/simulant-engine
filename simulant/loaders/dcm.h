#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum PositionFormat {
    POSITION_FORMAT_2F,
    POSITION_FORMAT_3F,
    POSITION_FORMAT_4F,
};

enum TexCoordFormat {
    TEX_COORD_FORMAT_NONE = 0,
    TEX_COORD_FORMAT_2F,
    TEX_COORD_FORMAT_2US,
};

enum ColorFormat {
    COLOR_FORMAT_NONE = 0,
    COLOR_FORMAT_4UB,
    COLOR_FORMAT_3F,
    COLOR_FORMAT_4F,
};

enum NormalFormat {
    NORMAL_FORMAT_NONE,
    NORMAL_FORMAT_3F,
};

enum SubMeshArrangement {
    SUB_MESH_ARRANGEMENT_TRIANGLE_STRIP,
    SUB_MESH_ARRANGEMENT_TRIANGLES,
};

enum SubMeshType {
    SUB_MESH_TYPE_RANGED,
    SUB_MESH_TYPE_INDEXED
};

/* Structure of the file:
 *
 * 1 x FileHeader
 * N x Materials
 * N x Meshes
 *   1 x MeshHeader
 *   N x Vertices
 *   N x Submeshes
 *     1 x SubMeshHeader
 *     N x Vertex Ranges
 */

#pragma pack(push, 1)

#define DCM_CURRENT_VERSION 0

struct FileHeader {
    uint8_t id[3];  /* Should be DCM */
    uint8_t version; /* Format version */
    uint8_t material_count; /* Number of materials stored in this file */
    uint8_t mesh_count;  /* Number of lod-meshes stored in this file */
    uint8_t pos_format; /* Format of the vertex positions */
    uint8_t tex0_format; /* Format of the vertex texture coordinates */
    uint8_t color_format; /* Format of the vertex colours */
    uint8_t offset_colour_format;  /* Format of the offset colour */
    uint8_t tex1_format; /* Format of the vertex texture coordinates */
    uint8_t normal_format; /* Format of the supplied normals */
    uint8_t vertex_size; /* Can be calculated from the above formats */
    uint8_t index_size;  /* Size of indices in bytes (1, 2, 4) */
    uint32_t mesh_offset;  /* Offset from the start of the file to the first mesh */
    uint32_t reserved0;
    uint32_t reserved1;
};

struct Material {
    char name[32];
    float ambient[4] = {0, 0, 0, 1};
    float diffuse[4] = {0, 0, 0, 1};  /* Diffuse in RGBA order */
    float specular[4] = {0, 0, 0, 1};
    float emission[4] = {0, 0, 0, 1};
    float shininess;
    char diffuse_map[32];  /* Filename of the diffuse map, if byte zero is \0 then there is no diffuse map */
    char light_map[32]; /* Filename of the light map, if byte zero is \0 then there is no light map */
    char normal_map[32]; /* Filename of the normal map, if byte zero is \0 then there is no normal map */
    char specular_map[32]; /* Filename of the specular map, if byte zero is \0 then there is no specular map */
};

struct MeshHeader {
    char name[32];
    uint8_t submesh_count; /* Number of submeshes that follow the vertex data */
    uint8_t reserved[3];  /* Potentially bone count etc. */
    uint32_t vertex_count; /* Number of vertices that follow this header */
    uint32_t first_submesh_offset; /* Offset from the start of the file to the first submesh */
    uint32_t next_mesh_offset; /* Offset from the start of the file to the next mesh */
};

struct SubMeshHeader {
    uint8_t material_id; /* Index to the materials list */
    uint8_t arrangement; /* Strips or triangles */
    uint8_t type; /* Whether vertex ranges, or indexes follow this struct */
    uint16_t num_ranges_or_indices; /* Number of submesh vertex ranges or indices that follow */
    uint32_t next_submesh_offset; /* Offset frmo the start of the file to the next submesh */
};

struct SubMeshVertexRange {
    uint32_t start;  /* Index in the mesh's vertex list to start from */
    uint32_t count;  /* Number of vertices to render from the start of the list */
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
