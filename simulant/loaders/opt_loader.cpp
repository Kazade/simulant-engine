//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <queue>

#include "../meshes/mesh.h"
#include "../texture.h"
#include "../asset_manager.h"
#include "../shortcuts.h"
#include "opt_loader.h"

namespace smlt {
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
    OFFSET_BLOCK = 0,
    FACE = 1,
    VERTEX = 3,
    REUSED_TEXTURE = 7,
    VERTEX_NORMAL = 11,
    TEXTURE_VERTEX = 13,
    EMBEDDED_TEXTURE = 20,
    LOD = 21,
    TEXTURE_OFFSET_BLOCK = 24
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

struct OffsetDataBlock {
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

typedef int32_t Offset;

void OPTLoader::process_vertex_block(std::istream& file) {
    VertexDataBlock vertex_data_block;
    file.read((char*)&vertex_data_block, sizeof(VertexDataBlock));
    vertex_data_block.offset_to_vertices -= global_offset;
    file.seekg(vertex_data_block.offset_to_vertices, std::ios_base::beg);

    vertices.resize(vertex_data_block.vertex_count);
    file.read((char*)&vertices[0], sizeof(Vec3)* vertex_data_block.vertex_count);
    S_DEBUG("Loaded {0} vertices", vertex_data_block.vertex_count);
}

void OPTLoader::process_texcoord_block(std::istream& file) {
    TextureVertexDataBlock tv_data_block;
    file.read((char*)&tv_data_block, sizeof(TextureVertexDataBlock));
    tv_data_block.offset_to_texture_vertices -= global_offset;
    file.seekg(tv_data_block.offset_to_texture_vertices, std::ios_base::beg);

    texture_vertices.resize(tv_data_block.texture_vertex_count);
    file.read((char*)&texture_vertices[0], sizeof(float) * tv_data_block.texture_vertex_count * 2);
    S_DEBUG("Loaded {0} texture vertices", tv_data_block.texture_vertex_count);
}

void OPTLoader::process_normal_block(std::istream& file) {
    VertexNormalDataBlock vertex_normal_data_block;
    file.read((char*)&vertex_normal_data_block, sizeof(VertexNormalDataBlock));
    vertex_normal_data_block.offset_to_vertex_normals -= global_offset;
    file.seekg(vertex_normal_data_block.offset_to_vertex_normals, std::ios_base::beg);

    vertex_normals.resize(vertex_normal_data_block.vertex_normal_count);
    file.read((char*)&vertex_normals[0], sizeof(Vec3)* vertex_normal_data_block.vertex_normal_count);
    S_DEBUG("Loaded {0} vertex normals", vertex_normal_data_block.vertex_normal_count);
}

void OPTLoader::process_reused_texture_block(std::istream& file) {
    int32_t offset = file.tellg();

    EmbeddedTextureDataBlockHeader header_info;
    file.read((char*)&header_info, sizeof(EmbeddedTextureDataBlockHeader));
    current_texture = std::string(header_info.texture_name, header_info.texture_name + 8);

    S_DEBUG("Found *reused* texture {0} - offset: {1}", current_texture, offset);
}

void OPTLoader::process_lod_block(std::istream& file) {
    LODDataBlock lod_data_block;
    file.read((char*)&lod_data_block, sizeof(LODDataBlock));
    lod_data_block.offset_to_face_data_header_jumps -= global_offset;
    lod_data_block.offset_to_lod_distances -= global_offset;
    file.seekg(lod_data_block.offset_to_face_data_header_jumps, std::ios_base::beg);

    std::vector<int32_t> new_offsets(lod_data_block.face_data_header_count, 0);
    file.read((char*)&new_offsets[0], sizeof(int32_t) * lod_data_block.face_data_header_count);

    //Resize the triangles array to contain all the levels of detail
    triangles.resize(lod_data_block.face_data_header_count);

    for(int32_t new_offset: new_offsets) {
        if(new_offset == 0) continue;

        //Read this LOD and child blocks
        read_block(file, new_offset - global_offset);

        //Increment the current LOD for the next iteration
        current_lod++;
    }
}

void OPTLoader::process_face_block(std::istream& file) {
    FaceDataBlock face_data_block;
    file.read((char*)&face_data_block, sizeof(FaceDataBlock));
    face_data_block.offset_to_face_data -= global_offset;
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

        assert(vertex_indices[0] < (int32_t) vertices.size());
        assert(vertex_indices[1] < (int32_t) vertices.size());
        assert(vertex_indices[2] < (int32_t) vertices.size());
        if(vertex_indices.size() == 4) {
            assert(vertex_indices[3] < (int32_t) vertices.size());
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

        assert(texture_coordinates[0] < (int32_t) texture_vertices.size());
        assert(texture_coordinates[1] < (int32_t) texture_vertices.size());
        assert(texture_coordinates[2] < (int32_t) texture_vertices.size());
        if(texture_coordinates.size() == 4) {
            assert(texture_coordinates[3] < (int32_t) texture_vertices.size());
        }

        std::vector<int32_t> vertex_normal_indexes(4, -1);
        file.read((char*)&vertex_normal_indexes[0], sizeof(int32_t) * 4);
        vertex_normal_indexes.erase(
            std::remove_if(vertex_normal_indexes.begin(), vertex_normal_indexes.end(), [](int32_t x){ return x == -1; }),
            vertex_normal_indexes.end()
        );

        assert(vertex_normal_indexes[0] < (int32_t) vertex_normals.size() && vertex_normal_indexes[0] >= 0);
        assert(vertex_normal_indexes[1] < (int32_t) vertex_normals.size() && vertex_normal_indexes[1] >= 0);
        assert(vertex_normal_indexes[2] < (int32_t) vertex_normals.size() && vertex_normal_indexes[2] >= 0);
        if(vertex_normal_indexes.size() == 4) {
            assert(vertex_normal_indexes[3] < (int32_t) vertex_normals.size() && vertex_normal_indexes[3] >= 0);
        }

        if(vertex_indices.size() == 3) {
            Triangle new_face;
            new_face.texture_name = current_texture;

            for(int8_t i = 0; i < 3; ++i) {
                new_face.positions[i] = vertices[vertex_indices[i]];
                new_face.tex_coords[i] = texture_vertices[texture_coordinates[i]];
                new_face.normals[i] = vertex_normals[vertex_normal_indexes[i]];
            }

            triangles[current_lod].push_back(new_face);
        } else {
            Triangle t1, t2;
            t1.texture_name = current_texture;
            t2.texture_name = current_texture;

            int8_t j = 0;
            for(int8_t i: { 0, 1, 2 }) {
                t1.positions[j] = vertices[vertex_indices[i]];
                t1.tex_coords[j] = texture_vertices[texture_coordinates[i]];
                t1.normals[j] = vertex_normals[vertex_normal_indexes[i]];
                ++j;
            }

            j = 0;
            for(int8_t i: { 0, 2, 3 }) {
                t2.positions[j] = vertices[vertex_indices[i]];
                t2.tex_coords[j] = texture_vertices[texture_coordinates[i]];
                t2.normals[j] = vertex_normals[vertex_normal_indexes[i]];
                ++j;
            }

            triangles[current_lod].push_back(t1);
            triangles[current_lod].push_back(t2);
        }
    }
}

void OPTLoader::process_embedded_texture_block(std::istream& file) {
    EmbeddedTextureDataBlock texture_data_block;
    file.read((char*)&texture_data_block.header, sizeof(EmbeddedTextureDataBlockHeader));
    texture_data_block.header.offset_to_palette_data_offset -= global_offset;

    //If there are alpha masks in this texture, push the offsets onto the queue (we can use
    // current_texture to work out which alpha mask belongs to which texture)
    for(uint8_t i = 0; i < texture_data_block.header.alpha_mask_count; ++i) {
        int32_t alpha_mask_offset;
        file.read((char*)&alpha_mask_offset, sizeof(int32_t));
        alpha_mask_offset -= global_offset;
        read_block(file, alpha_mask_offset);
    }

    //Make sure we are looking in the right place
    file.seekg(texture_data_block.header.offset_to_palette_data_offset, std::ios_base::beg);
    file.read((char*)&texture_data_block.data, sizeof(EmbeddedTextureDataBlockData));

    texture_data_block.data.offset_to_palette_data -= global_offset;

    //Read the image data following the data block
    std::vector<uint8_t> image_data = std::vector<uint8_t>(texture_data_block.data.data_size);
    file.read((char*)&image_data[0], sizeof(uint8_t) * texture_data_block.data.data_size);

    std::string texture_name = std::string(texture_data_block.header.texture_name, texture_data_block.header.texture_name + 8);
    S_DEBUG("Found texture with ID {0} and name '{1}'", texture_data_block.header.texture_id, texture_name);

    Texture new_texture;
    new_texture.name = texture_name;
    new_texture.width = texture_data_block.data.width;
    new_texture.height = texture_data_block.data.height;
    new_texture.bytes_per_pixel = 3;

    uint16_t palette_data[256];
    file.seekg(texture_data_block.data.offset_to_palette_data);

    // Seek past 15 shade tables to get to the actual palette
    file.seekg(sizeof(uint16_t) * 256 * 15, std::ios_base::cur);

    // Read in the palette data
    file.read((char*)&palette_data, sizeof(uint16_t) * 256);

    new_texture.data.resize(new_texture.width * new_texture.height * new_texture.bytes_per_pixel);

    for(int32_t i = 0; i < new_texture.width * new_texture.height; ++i) {
        uint8_t palette_index = image_data[i];
        uint16_t entry = palette_data[palette_index];

        uint32_t b = (entry & 0xF800) >> 11;
        uint32_t g = (entry & 0x07E0) >> 5;
        uint32_t r = (entry & 0x001F);

        b = b * (255.0f / 31.0f);
        g = g * (255.0f / 63.0f);
        r = r * (255.0f / 31.0f);

        new_texture.data[i * new_texture.bytes_per_pixel] = r;
        new_texture.data[(i * new_texture.bytes_per_pixel) + 1] = g;
        new_texture.data[(i * new_texture.bytes_per_pixel) + 2] = b;
    }

    textures.push_back(new_texture);

    current_texture = texture_name;
}

void OPTLoader::read_block(std::istream& file, Offset offset) {
    uint8_t team = 2;

    DataBlockHeader data_block_header;

    file.seekg(offset, std::ios_base::beg);
    file.read((char*)&data_block_header, sizeof(DataBlockHeader));

    if(data_block_header.type > 0 && data_block_header.type != TEXTURE_OFFSET_BLOCK) {
        S_DEBUG("Processing data block: {0}", data_block_header.type);
        switch(data_block_header.type) {
            case DataBlockTypes::VERTEX:
            process_vertex_block(file);
            break;
            case DataBlockTypes::TEXTURE_VERTEX:
            process_texcoord_block(file);
            break;
            case DataBlockTypes::VERTEX_NORMAL:
            process_normal_block(file);
            break;
            case DataBlockTypes::REUSED_TEXTURE:
            process_reused_texture_block(file);
            break;
            case DataBlockTypes::LOD:
            process_lod_block(file);
            break;
            case DataBlockTypes::FACE:
            process_face_block(file);
            break;
            case DataBlockTypes::EMBEDDED_TEXTURE:
            process_embedded_texture_block(file);
            break;
            default:
                S_WARN("Unhandled block type: {0}", data_block_header.type);
        }
    } else {
        S_DEBUG("Processing jump block: {0}", data_block_header.type);
        OffsetDataBlock mesh_info_data_block;
        file.read((char*)&mesh_info_data_block, sizeof(OffsetDataBlock));
        mesh_info_data_block.offset_to_block_offsets -= global_offset;
        file.seekg(mesh_info_data_block.offset_to_block_offsets, std::ios_base::beg);

        //Initialize the offset array to zero, but make it the right size
        std::vector<int32_t> new_block_offsets(mesh_info_data_block.block_offset_count, 0);
        //Read the list of offsets
        file.read((char*)&new_block_offsets[0], sizeof(int32_t) * mesh_info_data_block.block_offset_count);

        if(data_block_header.type == TEXTURE_OFFSET_BLOCK) {
            //Texture offset blocks look and behave like a normal offset block except they have
            // normally 4 textures which represent different colors. We just pick one based on the
            // team variable
            int8_t team_skin_offset_index = std::min<int8_t>(new_block_offsets.size() - 1, team);
            read_block(file, new_block_offsets[team_skin_offset_index] - global_offset);
        } else {
            //Add any new block offsets to the end of the block_offsets list that we are iterating
            for(int32_t block_offset: new_block_offsets) {
                if(block_offset == 0) {
                    S_DEBUG("Skipping NULL offset");
                    continue; //Ignore null offsets
                }
                int32_t final = block_offset - global_offset;
                read_block(file, final);
            }
        }
    }
}

void OPTLoader::into(Loadable& resource, const LoaderOptions &options) {
    _S_UNUSED(options);

    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);
    assert(mesh && "You passed a Resource that is not a mesh to the OPT loader");

    std::ifstream file(filename_.str(), std::ios::binary);
    if(!file.good()) {
        throw std::runtime_error("Couldn't load the OPT file: " + filename_.str());
    }

    VertexSpecification spec;
    spec.position_attribute = VERTEX_ATTRIBUTE_3F;
    spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    spec.texcoord1_attribute = VERTEX_ATTRIBUTE_2F;
    spec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;
    spec.normal_attribute = VERTEX_ATTRIBUTE_3F;

    mesh->reset(spec);

    MainHeader main_header;
    MainJumpHeader main_jump_header;

    //Read the main headers
    file.read((char*)&main_header, sizeof(MainHeader));
    file.read((char*)&main_jump_header, sizeof(MainJumpHeader));

    //The global offset is 8 too large to use (weird...)
    main_header.global_offset -= 8;
    global_offset = main_header.global_offset;

    S_DEBUG("Global offset is {0}", main_header.global_offset);

    //All offsets need to have the global offset added
    main_jump_header.offset_to_mesh_header_offsets -= main_header.global_offset;

    file.seekg(main_jump_header.offset_to_mesh_header_offsets, std::ios_base::beg);

    //Following the jump header is a list of offsets to mesh headers
    //We read those here
    std::vector<Offset> offsets;
    for(uint16_t i = 0; i < main_jump_header.mesh_header_offset_count; ++i) {
        int32_t offset;
        file.read((char*)&offset, sizeof(int32_t));

        offsets.push_back(offset - global_offset);
    }

    for(Offset off: offsets) {
        vertices.clear();
        vertex_normals.clear();
        texture_vertices.clear();
        current_lod = 0;

        file.seekg(off, std::ios_base::beg);
        read_block(file, off);
    }

    for(Texture& tex: textures) {
        if(texture_name_to_id.count(tex.name)) continue;

        texture_name_to_id[tex.name] = mesh->asset_manager().new_texture(
            tex.width, tex.height,
            (tex.bytes_per_pixel == 3) ? TEXTURE_FORMAT_RGB_3UB_888 : TEXTURE_FORMAT_RGBA_4UB_8888
        );

        auto new_tex = texture_name_to_id[tex.name];
        new_tex->set_data(tex.data);

        //Create a submesh for each texture.
        texture_submesh[tex.name] = mesh->new_submesh(
            tex.name,
            mesh->asset_manager().new_material_from_texture(new_tex),
            INDEX_TYPE_16_BIT,
            MESH_ARRANGEMENT_TRIANGLES
        );
    }

    //Now let's build everything!
    for(Triangle tri: triangles[0]) {
        if(!texture_submesh.count(tri.texture_name)) {
            S_ERROR(
                "Some part of this file wasn't loaded, as we have found a reused texture {0} without loading the actual texture. Some of the model will be missing",
                tri.texture_name
            );
            continue;
        }
        SubMesh& submesh = *texture_submesh[tri.texture_name];

        mesh->vertex_data->move_to_end();

        Quaternion rotation(Degrees(-90), Degrees(180), Degrees(0));

        for(int8_t i = 0; i < 3; ++i) {
            Vec3 pos = tri.positions[i];
            Vec2 tex_coord = tri.tex_coords[i];
            Vec3 normal = tri.normals[i];

            pos *= rotation;
            normal *= rotation;

            /* X-Wings are apparently 12.5 meters long. The XWING.OPT model from XWA
             * has a length of 416 units, so we divide that by 33.3 to get to roughly
             * the right size
             */
            mesh->vertex_data->position(pos.x / 33.3f, pos.y / 33.3f, pos.z / 33.3f);
            mesh->vertex_data->tex_coord0(tex_coord);
            mesh->vertex_data->tex_coord1(tex_coord.x, tex_coord.y);
            mesh->vertex_data->diffuse(smlt::Color::WHITE);
            mesh->vertex_data->normal(normal.x, normal.y, normal.z);
            mesh->vertex_data->move_next();

            submesh.index_data->index(mesh->vertex_data->count() - 1);
        }
    }

    mesh->vertex_data->done();

    for(Texture tex: textures) {
        texture_submesh[tex.name]->index_data->done();
        texture_submesh[tex.name]->reverse_winding();
    }
}

}
}
