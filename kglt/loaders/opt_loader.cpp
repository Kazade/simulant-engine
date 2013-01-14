#include <queue>
#include "../mesh.h"
#include "../texture.h"
#include "../resource_manager.h"
#include "../shortcuts.h"

#include "kazbase/unicode/unicode.h"
#include "opt_loader.h"

namespace kglt {
namespace loaders {

#pragma pack(push, 1)
struct MainHeader {
    int32_t magic; //FB FF FF FF
    int32_t file_size;
    int32_t global_offset; //Must have 8 subtracted to be usable
};

struct MainJumpHeader {
    uint8_t unknown1; //Always 2
    uint8_t unknown2; //Always 0
    int32_t mesh_header_offset_count;
    int32_t offset_to_mesh_header_offsets;
};

struct MainMeshHeader {
    int32_t unknown1; //Always 0
    int32_t unknown2; //Always 0
    int32_t jump_count; //Number of block 22 + block 28 + 3
    int32_t offset_to_jump_offsets;
    int32_t unknown3; //Always 1
    int32_t global_reverse_offset;
};

enum DataBlockTypes {
    MESH_INFO = 0,
    FACE = 1,
    VERTEX = 3,
    EXTERNAL_TEXTURE = 7,
    VERTEX_NORMAL = 11,
    TEXTURE_VERTEX = 13,
    EMBEDDED_TEXTURE = 20,
    LOD = 21
};

struct DataBlockHeader {
    int32_t begin;
    int32_t type; //The block type, this is the important bit
};

struct VertexDataBlock {
    int32_t unknown1;
    int32_t unknown2;
    int32_t vertex_count;
    int32_t offset_to_vertices;
};

struct LODDataBlock {
    int32_t face_data_header_count;
    int32_t offset_to_face_data_header_jumps;
    int32_t face_data_header_count2; //???
    int32_t offset_to_lod_distances;
};

struct TextureVertexDataBlock {
    int32_t unknown1;
    int32_t unknown2;
    int32_t texture_vertex_count;
    int32_t offset_to_texture_vertices;
};

struct VertexNormalDataBlock {
    int32_t unknown1;
    int32_t unknown2;
    int32_t vertex_normal_count;
    int32_t offset_to_vertex_normals;
};

struct MeshInfoDataBlock {
    int32_t block_offset_count;
    int32_t offset_to_block_offsets;
    int32_t unknown1;
    int32_t reverse_offset_to_vertices;
};

struct FaceDataBlock {
    int32_t unknown1;
    int32_t unknown2;
    int32_t face_count;
    int32_t offset_to_face_data;
    int32_t edge_count;
};

struct EmbeddedTextureDataBlockHeader {
    int32_t alpha_mask_count; //Guessing...
    int32_t offset_to_alpha_mask_offset;
    int32_t texture_id;
    int32_t offset_to_palette_data_offset;
    uint8_t texture_name[9];
};

struct EmbeddedTextureDataBlockData {
    int32_t offset_to_palette_data;
    int32_t unknown1;
    int32_t texture_size; //width * height;
    int32_t data_size; //size including mipmap sizes
    int32_t width;
    int32_t height;
};

struct EmbeddedTextureDataBlock {
    EmbeddedTextureDataBlockHeader header;
    EmbeddedTextureDataBlockData data;
};

struct ExternalTextureDataBlock {
    int32_t unknown1;
    int32_t unknown2;
    int32_t unknown3;
    int32_t offset_to_texture_name;
    uint8_t texture_name[9];
};

#pragma pack(pop)

void OPTLoader::into(Loadable& resource) {
    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);
    assert(mesh && "You passed a Resource that is not a mesh to the OPT loader");

    std::ifstream file(filename_.c_str(), std::ios::binary);
    if(!file.good()) {
        throw std::runtime_error("Couldn't load the OPT file: " + filename_);
    }

    MainHeader main_header;
    MainJumpHeader main_jump_header;

    //Read the main headers
    file.read((char*)&main_header, sizeof(MainHeader));
    file.read((char*)&main_jump_header, sizeof(MainJumpHeader));

    //The global offset is 8 too large to use (weird...)
    main_header.global_offset -= 8;

    //All offsets need to have the global offset added
    main_jump_header.offset_to_mesh_header_offsets -= main_header.global_offset;

    file.seekg(main_jump_header.offset_to_mesh_header_offsets, std::ios_base::beg);

    //Following the jump header is a list of offsets to mesh headers
    //We read those here
    std::queue<int32_t> main_offsets;
    for(uint16_t i = 0; i < main_jump_header.mesh_header_offset_count; ++i) {
        int32_t offset;
        file.read((char*)&offset, sizeof(int32_t));
        main_offsets.push(offset - main_header.global_offset); //Again, add the global offset
    }


    struct Triangle {
        int32_t vertex_indexes[3];
        int32_t texture_indexes[3];
        int32_t normal_indexes[3];

        Vec3 face_normal;
        std::string texture_name;
    };

    struct Texture {
        std::string name;
        int32_t width;
        int32_t height;
        int32_t bytes_per_pixel;
        std::vector<uint8_t> data;
    };

    std::vector<Texture> textures;
    std::map<std::string, SubMeshIndex> texture_submesh;
    std::map<std::string, TextureID> texture_name_to_id;

    std::string current_texture;

    while(!main_offsets.empty()) {
        std::queue<int32_t> block_offsets;
        block_offsets.push(main_offsets.front());
        main_offsets.pop();

        //Temporary data store
        std::vector<Vec3> vertices;
        std::vector<Vec2> texture_vertices;
        std::vector<Vec3> vertex_normals;

        std::vector<Triangle> triangles;

        while(!block_offsets.empty()) {
            //Grab the next offset from the queue
            int32_t offset = block_offsets.front();
            block_offsets.pop();

            file.seekg(offset, std::ios_base::beg);
            DataBlockHeader data_header;
            file.read((char*)&data_header, sizeof(DataBlockHeader));

            L_DEBUG(unicode("Handling block type: {0}").format(data_header.type).encode());
            switch(data_header.type) {
                case DataBlockTypes::EMBEDDED_TEXTURE: {
                    EmbeddedTextureDataBlock texture_data_block;
                    file.read((char*)&texture_data_block.header, sizeof(EmbeddedTextureDataBlockHeader));
                    texture_data_block.header.offset_to_palette_data_offset -= main_header.global_offset;

                    //If there are alpha masks in this texture, push the offsets onto the queue (we can use
                    // current_texture to work out which alpha mask belongs to which texture)
                    for(uint8_t i = 0; i < texture_data_block.header.alpha_mask_count; ++i) {
                        int32_t alpha_mask_offset;
                        file.read((char*)&alpha_mask_offset, sizeof(int32_t));
                        alpha_mask_offset -= main_header.global_offset;
                        block_offsets.push(alpha_mask_offset);
                    }

                    //Make sure we are looking in the right place
                    file.seekg(texture_data_block.header.offset_to_palette_data_offset, std::ios_base::beg);
                    file.read((char*)&texture_data_block.data, sizeof(EmbeddedTextureDataBlockData));

                    texture_data_block.data.offset_to_palette_data -= main_header.global_offset;

                    //Read the image data following the data block
                    std::vector<uint8_t> image_data = std::vector<uint8_t>(texture_data_block.data.data_size);
                    file.read((char*)&image_data[0], sizeof(uint8_t) * texture_data_block.data.data_size);

                    std::string texture_name = std::string(texture_data_block.header.texture_name, texture_data_block.header.texture_name + 8);
                    L_DEBUG(unicode("Found texture with ID {0} and name '{1}'").format(texture_data_block.header.texture_id, texture_name).encode());

                    Texture new_texture;
                    new_texture.name = texture_name;
                    new_texture.width = texture_data_block.data.width;
                    new_texture.height = texture_data_block.data.height;
                    new_texture.bytes_per_pixel = 4; //FIXME:

                    uint16_t palette_data[256];
                    file.seekg(texture_data_block.data.offset_to_palette_data);
                    file.seekg(256 * 14, std::ios_base::cur);
                    file.read((char*)&palette_data, sizeof(uint16_t) * 256);

                    new_texture.data.resize(new_texture.width * new_texture.height * new_texture.bytes_per_pixel);
                    for(int32_t i = 0; i < new_texture.width * new_texture.height; ++i) {
                        uint8_t palette_index = image_data[i];
                        uint16_t entry = palette_data[palette_index];

                        uint32_t b = (entry & 0xF800) >> 11;
                        uint32_t g = (entry & 0x07E0) >> 5;
                        uint32_t r = (entry & 0x001F);

                        b = b * 255 / 31;
                        g = g * 255 / 63;
                        r = r * 255 / 31;

                        new_texture.data[i * new_texture.bytes_per_pixel] = r;
                        new_texture.data[i * new_texture.bytes_per_pixel + 1] = g;
                        new_texture.data[i * new_texture.bytes_per_pixel + 2] = b;
                        new_texture.data[i * new_texture.bytes_per_pixel + 3] = 255;
                    }

                    textures.push_back(new_texture);

                    current_texture = texture_name;
                } break;
                case DataBlockTypes::FACE: {
                    FaceDataBlock face_data_block;
                    file.read((char*)&face_data_block, sizeof(FaceDataBlock));
                    face_data_block.offset_to_face_data -= main_header.global_offset;
                    file.seekg(face_data_block.offset_to_face_data, std::ios_base::beg);

                    int32_t unknown; //Texture ID?
                    file.read((char*)&unknown, sizeof(int32_t));

                    int32_t face_data_start = file.tellg();
                    for(int32_t j = 0; j < face_data_block.face_count; ++j) {
                        //4 indexes for vertices, edges, texcoords, normals + face normal + texture info
                        int32_t stride = 64;
                        file.seekg(face_data_start + (j * stride));

                        std::vector<int32_t> vertex_indices(4, -1);
                        file.read((char*)&vertex_indices[0], sizeof(int32_t) * 4);
                        vertex_indices.erase(
                            std::remove_if(vertex_indices.begin(), vertex_indices.end(), [](int32_t x){ return x == -1; }),
                            vertex_indices.end()
                        );

                        assert(vertex_indices[0] < vertices.size());
                        assert(vertex_indices[1] < vertices.size());
                        assert(vertex_indices[2] < vertices.size());
                        if(vertex_indices.size() == 4) {
                            assert(vertex_indices[3] < vertices.size());
                        }

                        std::vector<int32_t> edges(4, -1);
                        file.read((char*)&edges[0], sizeof(int32_t) * 4);
                        edges.erase(
                            std::remove_if(edges.begin(), edges.end(), [](int32_t x){ return x == -1; }),
                            edges.end()
                        );

                        std::vector<int32_t> texture_coordinates(4, -1);
                        file.read((char*)&texture_coordinates[0], sizeof(int32_t) * 4);
                        texture_coordinates.erase(
                            std::remove_if(texture_coordinates.begin(), texture_coordinates.end(), [](int32_t x){ return x == -1; }),
                            texture_coordinates.end()
                        );

                        assert(texture_coordinates[0] < texture_vertices.size());
                        assert(texture_coordinates[1] < texture_vertices.size());
                        assert(texture_coordinates[2] < texture_vertices.size());
                        if(texture_coordinates.size() == 4) {
                            assert(texture_coordinates[3] < texture_vertices.size());
                        }

                        std::vector<int32_t> vertex_normal_indexes(4, -1);
                        file.read((char*)&vertex_normal_indexes[0], sizeof(int32_t) * 4);
                        vertex_normal_indexes.erase(
                            std::remove_if(vertex_normal_indexes.begin(), vertex_normal_indexes.end(), [](int32_t x){ return x == -1; }),
                            vertex_normal_indexes.end()
                        );

                        assert(vertex_normal_indexes[0] < vertex_normals.size() && vertex_normal_indexes[0] >= 0);
                        assert(vertex_normal_indexes[1] < vertex_normals.size() && vertex_normal_indexes[1] >= 0);
                        assert(vertex_normal_indexes[2] < vertex_normals.size() && vertex_normal_indexes[2] >= 0);
                        if(vertex_normal_indexes.size() == 4) {
                            assert(vertex_normal_indexes[3] < vertex_normals.size() && vertex_normal_indexes[3] >= 0);
                        }

                        if(vertex_indices.size() == 3) {
                            Triangle new_face;
                            new_face.texture_name = current_texture;
                            std::copy(vertex_indices.begin(), vertex_indices.end(), new_face.vertex_indexes);
                            std::copy(texture_coordinates.begin(), texture_coordinates.end(), new_face.texture_indexes);
                            std::copy(vertex_normal_indexes.begin(), vertex_normal_indexes.end(), new_face.normal_indexes);
                            triangles.push_back(new_face);
                        } else {
                            Triangle t1, t2;
                            t1.texture_name = current_texture;
                            t2.texture_name = current_texture;

                            t1.vertex_indexes[0] = vertex_indices[0];
                            t1.vertex_indexes[1] = vertex_indices[1];
                            t1.vertex_indexes[2] = vertex_indices[2];
                            t1.texture_indexes[0] = texture_coordinates[0];
                            t1.texture_indexes[1] = texture_coordinates[1];
                            t1.texture_indexes[2] = texture_coordinates[2];
                            t1.normal_indexes[0] = vertex_normal_indexes[0];
                            t1.normal_indexes[1] = vertex_normal_indexes[1];
                            t1.normal_indexes[2] = vertex_normal_indexes[2];

                            t2.vertex_indexes[0] = vertex_indices[0];
                            t2.vertex_indexes[1] = vertex_indices[2];
                            t2.vertex_indexes[2] = vertex_indices[3];
                            t2.texture_indexes[0] = texture_coordinates[0];
                            t2.texture_indexes[1] = texture_coordinates[2];
                            t2.texture_indexes[2] = texture_coordinates[3];
                            t2.normal_indexes[0] = vertex_normal_indexes[0];
                            t2.normal_indexes[1] = vertex_normal_indexes[2];
                            t2.normal_indexes[2] = vertex_normal_indexes[3];

                            triangles.push_back(t1);
                            triangles.push_back(t2);
                        }
                    }

                } break;
                case DataBlockTypes::MESH_INFO: {
                    MeshInfoDataBlock mesh_info_data_block;
                    file.read((char*)&mesh_info_data_block, sizeof(MeshInfoDataBlock));
                    mesh_info_data_block.offset_to_block_offsets -= main_header.global_offset;
                    file.seekg(mesh_info_data_block.offset_to_block_offsets, std::ios_base::beg);

                    //Initialize the offset array to zero, but make it the right size
                    std::vector<int32_t> new_block_offsets(mesh_info_data_block.block_offset_count, 0);
                    //Read the list of offsets
                    file.read((char*)&new_block_offsets[0], sizeof(int32_t) * mesh_info_data_block.block_offset_count);

                    //Add any new block offsets to the end of the block_offsets list that we are iterating
                    for(int32_t block_offset: new_block_offsets) {
                        if(block_offset == 0) continue; //Ignore null offsets
                        int32_t final = block_offset - main_header.global_offset;
                        block_offsets.push(final);
                        L_DEBUG(unicode("Adding offset to the list: {0}").format(final).encode());
                    }
                } break;
                case DataBlockTypes::VERTEX: {
                    VertexDataBlock vertex_data_block;
                    file.read((char*)&vertex_data_block, sizeof(VertexDataBlock));
                    vertex_data_block.offset_to_vertices -= main_header.global_offset;
                    file.seekg(vertex_data_block.offset_to_vertices, std::ios_base::beg);

                    vertices.resize(vertex_data_block.vertex_count);
                    file.read((char*)&vertices[0], sizeof(Vec3)* vertex_data_block.vertex_count);
                    L_DEBUG(unicode("Loaded {0} vertices").format(vertex_data_block.vertex_count).encode());
                } break;
                case DataBlockTypes::TEXTURE_VERTEX: {
                    TextureVertexDataBlock tv_data_block;
                    file.read((char*)&tv_data_block, sizeof(TextureVertexDataBlock));
                    tv_data_block.offset_to_texture_vertices -= main_header.global_offset;
                    file.seekg(tv_data_block.offset_to_texture_vertices, std::ios_base::beg);

                    texture_vertices.resize(tv_data_block.texture_vertex_count);
                    file.read((char*)&texture_vertices[0], sizeof(Vec2)* tv_data_block.texture_vertex_count);
                    L_DEBUG(unicode("Loaded {0} texture vertices").format(tv_data_block.texture_vertex_count).encode());
                } break;
                case DataBlockTypes::VERTEX_NORMAL: {
                    VertexNormalDataBlock vertex_normal_data_block;
                    file.read((char*)&vertex_normal_data_block, sizeof(VertexNormalDataBlock));
                    vertex_normal_data_block.offset_to_vertex_normals -= main_header.global_offset;
                    file.seekg(vertex_normal_data_block.offset_to_vertex_normals, std::ios_base::beg);

                    vertex_normals.resize(vertex_normal_data_block.vertex_normal_count);
                    file.read((char*)&vertex_normals[0], sizeof(Vec3)* vertex_normal_data_block.vertex_normal_count);
                    L_DEBUG(unicode("Loaded {0} vertex normals").format(vertex_normal_data_block.vertex_normal_count).encode());
                } break;
                case DataBlockTypes::LOD: {
                    LODDataBlock lod_data_block;
                    file.read((char*)&lod_data_block, sizeof(LODDataBlock));
                    lod_data_block.offset_to_face_data_header_jumps -= main_header.global_offset;
                    lod_data_block.offset_to_lod_distances -= main_header.global_offset;
                    file.seekg(lod_data_block.offset_to_face_data_header_jumps, std::ios_base::beg);

                    std::vector<int32_t> new_offsets(lod_data_block.face_data_header_count, 0);
                    file.read((char*)&new_offsets[0], sizeof(int32_t) * lod_data_block.face_data_header_count);
                    for(int32_t new_offset: new_offsets) {
                        if(new_offset == 0) continue;

                        //Add the new offset to the iteration list
                        block_offsets.push(new_offset - main_header.global_offset);

                        if(lod_data_block.face_data_header_count > 1) {
                            L_WARN("Multiple LOD levels are not currently handled");
                        }
                        break; //Remove this when they are
                    }

                } break;
            default:
                L_WARN(unicode("Unhandled block type: {0}").format(data_header.type).encode());
            }
        }

        for(Texture tex: textures) {
            if(container::contains(texture_name_to_id, tex.name)) continue;

            /*
             * FIXME: This is dirty, it loads the texture and material but doesn't bind them to the lifetime of the
             * mesh. This means they will be left behind if the user deletes the mesh, and the user has no easy way
             * to delete them (aside from inspecting the mesh itself)
             */
            texture_name_to_id[tex.name] = mesh->resource_manager().new_texture();

            kglt::Texture& new_tex = mesh->resource_manager().texture(texture_name_to_id[tex.name]);
            new_tex.resize(tex.width, tex.height);
            new_tex.set_bpp(tex.bytes_per_pixel * 8);
            new_tex.data().assign(tex.data.begin(), tex.data.end());
            new_tex.flip_vertically();
            new_tex.upload(true, true);

            //Create a submesh for each texture. Don't share the vertex data between submeshes
            texture_submesh[tex.name] = mesh->new_submesh(create_material_from_texture(mesh->resource_manager(), new_tex.id()), MESH_ARRANGEMENT_TRIANGLES, false);
        }

        //Now let's build everything!
        /* This could be optimized, there is no vertex sharing happening */
        for(Triangle tri: triangles) {
            SubMesh& submesh = mesh->submesh(texture_submesh[tri.texture_name]);

            submesh.vertex_data().move_to_end();

            for(int8_t i = 0; i < 3; ++i) {
                Vec3 pos = vertices[tri.vertex_indexes[i]];
                Vec2 tex_coord = texture_vertices[tri.texture_indexes[i]];
                Vec3 normal = vertex_normals[tri.normal_indexes[i]];

                submesh.vertex_data().position(pos.x, pos.z, pos.y);
                submesh.vertex_data().tex_coord0(tex_coord);
                submesh.vertex_data().tex_coord1(tex_coord.x, tex_coord.y);
                submesh.vertex_data().diffuse(kglt::Colour::white);
                submesh.vertex_data().normal(normal);
                submesh.vertex_data().move_next();
                submesh.index_data().index(submesh.vertex_data().count()-1);
            }
        }

        for(Texture tex: textures) {
            mesh->submesh(texture_submesh[tex.name]).vertex_data().done();
            mesh->submesh(texture_submesh[tex.name]).index_data().done();
        }
    }
}

}
}
