#pragma once

#include <cstdint>

namespace smlt {

enum PositionFormat : uint8_t {
    POSITION_FORMAT_2F,
    POSITION_FORMAT_3F,
    POSITION_FORMAT_4F,
};

enum TexCoordFormat : uint8_t {
    TEX_COORD_FORMAT_NONE = 0,
    TEX_COORD_FORMAT_2F,
    TEX_COORD_FORMAT_2US,
};

enum ColorFormat : uint8_t {
    COLOR_FORMAT_NONE = 0,
    COLOR_FORMAT_3UB,
    COLOR_FORMAT_4UB,
    COLOR_FORMAT_3F,
    COLOR_FORMAT_4F,
};

enum NormalFormat : uint8_t {
    NORMAL_FORMAT_3F,
};

enum SubMeshArrangement : uint8_t {
    SUB_MESH_ARRANGEMENT_TRIANGLE_STRIP,
    SUB_MESH_ARRANGEMENT_TRIANGLES,
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

struct FileHeader {
    uint8_t version = 1; /* Format version */
    uint8_t material_count; /* Number of materials stored in this file */
    uint8_t mesh_count;  /* Number of lod-meshes stored in this file */
    PositionFormat pos_format; /* Format of the vertex positions */
    TexCoordFormat tex0_format; /* Format of the vertex texture coordinates */
    ColorFormat color_format; /* Format of the vertex colours */
    ColorFormat offset_colour_format;  /* Format of the offset colour */
    TexCoordFormat tex1_format; /* Format of the vertex texture coordinates */
    NormalFormat normal_format; /* Format of the supplied normals */
    uint8_t vertex_size; /* Can be calculated from the above formats */
    uint32_t mesh_offset;  /* Offset from the start of the file to the first mesh */
};

struct Material {
    char name[16];
    uint8_t ambient[4];
    uint8_t diffuse[4];  /* Diffuse in RGBA order */
    uint8_t specular[4];
    uint8_t emissive[4];
    float shininess;
    char diffuse_map[32];  /* Filename of the diffuse map, if byte zero is \0 then there is no diffuse map */
    char normal_map[32]; /* Filename of the normal map, if byte zero is \0 then there is no normal map */
    char specular_map[32]; /* Filename of the specular map, if byte zero is \0 then there is no specular map */
};

struct MeshHeader {
    char name[16];
    uint8_t submesh_count; /* Number of submeshes that follow the vertex data */
    uint8_t reserved[3];  /* Potentially bone count etc. */
    uint32_t vdata_size;  /* Size of the following vertex data, in the format described in the file header */
    uint32_t next_mesh_offset; /* Offset from the start of the file to the next mesh */
};

struct SubMeshHeader {
    uint8_t material_id; /* Index to the materials list */
    SubMeshArrangement arrangement; /* Strips or triangles */
    uint16_t num_vertex_ranges; /* Number of submesh vertex ranges that follow */
    uint32_t next_submesh_offset; /* Offset frmo the start of the file to the next submesh */
};

struct SubMeshVertexRange {
    uint32_t start;  /* Index in the mesh's vertex list to start from */
    uint32_t count;  /* Number of vertices to render from the start of the list */
};

}
