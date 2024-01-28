#pragma once

#ifdef __cplusplus
#include <cstdint>
extern "C" {
namespace dcm {
#else
#include <stdint.h>
#endif

enum DataFlags {
    DATA_FLAG_EXTERNAL_LINK = 0x1,
};

enum PositionFormat {
    POSITION_FORMAT_NONE = 0,
    POSITION_FORMAT_2F,
    POSITION_FORMAT_3F,
    POSITION_FORMAT_4F,
};

enum RotationFormat {
    ROTATION_FORMAT_NONE = 0,
    ROTATION_FORMAT_QUAT_4F,
};

enum ScaleFormat {
    SCALE_FORMAT_NONE = 0,
    SCALE_FORMAT_3F,
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
    NORMAL_FORMAT_NONE = 0,
    NORMAL_FORMAT_3F,
};

enum BoneWeightsFormat {
    BONE_WEIGHTS_FORMAT_NONE = 0,
    BONE_WEIGHTS_FORMAT_3_UI_F,
};

enum KeyframeFormat { KEYFRAME_FORMAT_NONE = 0, KEYFRAME_FORMAT_2F = 1 };

enum SubMeshArrangement {
    SUB_MESH_ARRANGEMENT_NONE = 0,
    SUB_MESH_ARRANGEMENT_TRIANGLE_STRIP,
    SUB_MESH_ARRANGEMENT_TRIANGLES,
};

enum SubMeshType {
    SUB_MESH_TYPE_NONE = 0,
    SUB_MESH_TYPE_RANGED,
    SUB_MESH_TYPE_INDEXED
};

enum ChannelType {
    CHANNEL_TYPE_NONE = 0,
    CHANNEL_TYPE_POSITION_X = 1,
    CHANNEL_TYPE_POSITION_Y = 2,
    CHANNEL_TYPE_POSITION_Z = 3,
    CHANNEL_TYPE_ROTATION_QUAT_X = 4,
    CHANNEL_TYPE_ROTATION_QUAT_Y = 5,
    CHANNEL_TYPE_ROTATION_QUAT_Z = 6,
    CHANNEL_TYPE_ROTATION_QUAT_W = 7,
    CHANNEL_TYPE_SCALE_X = 8,
    CHANNEL_TYPE_SCALE_Y = 9,
    CHANNEL_TYPE_SCALE_Z = 10,

    CHANNEL_TYPE_USER_START, /* Types from here to CHANNEL_TYPE_LAST are
                                user-defined */
    CHANNEL_TYPE_LAST = 255
};

enum ChannelFlags {
    CHANNEL_FLAG_INTERP_CONSTANT = 0x01,
    CHANNEL_FLAG_INTERP_LINEAR = 0x02,

    /* Remaining flags are reserved */
    CHANNEL_FLAG_LAST = 0xFF
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
 * N x Armatures
 *   1 x ArmatureHeader
 *   N x Bones
 * N x Animations
 *   1 x AnimationHeader
 *   N x Channels
 *      1 x ChannelHeader
 *      N x Keyframes
 */

#define DCM_CURRENT_VERSION 1

#pragma pack(push, 1)

struct FileHeader {
    uint8_t id[3];               /* Should be DCM */
    uint8_t version;             /* Format version */
    uint8_t material_count;      /* Number of materials stored in this file */
    uint8_t mesh_count;          /* Number of lod-meshes stored in this file */
    uint8_t armature_count;      /* Number of armatures stored in this file */
    uint8_t animation_count;     /* Number of animations stored in this file */
    uint8_t pos_format;          /* Format of the vertex positions */
    uint8_t tex0_format;         /* Format of the vertex texture coordinates */
    uint8_t tex1_format;         /* Format of the vertex texture coordinates */
    uint8_t color_format;        /* Format of the vertex colors */
    uint8_t offset_color_format; /* Format of the offset color */
    uint8_t normal_format;       /* Format of the supplied normals */
    uint8_t index_size;          /* Size of indices in bytes (1, 2, 4) */
    uint8_t bone_weights_format; /* Format of the vertex bone weights */
};

struct DataHeader {
    uint8_t flags;    /* DataFlags affecting this data */
    uint8_t local_id; /* Local ID for this data */

    /**
     * Path for this data. Can be either internal ("my_data_item") or external
     * ("other_file.dcmesh@my_data_item"). In cases where nested data should be
     * accessed such as a bone within an armature, a forward slash separates
     * parent from child data ("other_file.dcmesh@my_data_item/my_data_part").
     * If DATA_FLAG_EXTERNAL_LINK is set, the path will be external - otherwise,
     * it's internal. Path refers to the data item within a dcmesh and is *not*
     * a filesystem path.
     */
    char path[128];
};

struct Material {
    DataHeader data_header;
    float ambient[4] = {0, 0, 0, 1};  /* RGBA order */
    float diffuse[4] = {0, 0, 0, 1};  /* RGBA order */
    float specular[4] = {0, 0, 0, 1}; /* RGBA order */
    float emission[4] = {0, 0, 0, 1}; /* RGBA order */
    float shininess;      /* Specular exponent as per OpenGL GL_SHININESS */
    char diffuse_map[32]; /* Filename of the diffuse map, if byte zero is \0
                             then there is no diffuse map */
    char light_map[32];   /* Filename of the light map, if byte zero is \0 then
                             there is no light map */
    char normal_map[32];  /* Filename of the normal map, if byte zero is \0 then
                             there is no normal map */
    char specular_map[32]; /* Filename of the specular map, if byte zero is \0
                              then there is no specular map */
};

struct ArmatureHeader {
    DataHeader data_header;
    uint8_t bone_count;
};

struct BoneHeader {
    DataHeader data_header;
};

struct AnimationHeader {
    DataHeader data_header;
    uint8_t channel_count;
};

struct ChannelHeader {
    uint8_t flags;
    uint16_t keyframe_count;
    uint8_t channel_type; /* Which type of data this channel represents */
    char target_path[32]; /* The target that the bone should affect
                             (engine-specific, but could be the path of a bone
                             for example) */
};

struct MeshHeader {
    DataHeader data_header;
    uint8_t submesh_count; /* Number of submeshes that follow the vertex data */
    uint32_t vertex_count; /* Number of vertices that follow this header */
};

struct SubMeshHeader {
    DataHeader data_header;
    uint8_t material_id; /* Local ID of material used by this submesh */
    uint8_t arrangement; /* Strips or triangles */
    uint8_t type; /* Whether vertex ranges, or indexes follow this struct */
    uint16_t num_ranges_or_indices; /* Number of submesh vertex ranges or
                                       indices that follow */
};

struct SubMeshVertexRange {
    uint32_t start; /* Index in the mesh's vertex list to start from */
    uint32_t
        count; /* Number of vertices to render from the start of the list */
};

struct VertexWeight_UI_F {
    uint32_t id;
    float influence;
};

#pragma pack(pop)

#ifdef __cplusplus
} // namespace dcm
} // extern "C"
#endif
