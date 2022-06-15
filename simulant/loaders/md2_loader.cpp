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


#include "md2_loader.h"
#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "../vfs.h"
#include "../time_keeper.h"

namespace smlt {
namespace loaders{

struct MD2Header {
    int32_t ident;                  /* magic number: "IDP2" */
    int32_t version;                /* version: must be 8 */

    int32_t skinwidth;              /* texture width */
    int32_t skinheight;             /* texture height */

    int32_t framesize;              /* size in bytes of a frame */

    int32_t num_skins;              /* number of skins */
    int32_t num_vertices;           /* number of vertices per frame */
    int32_t num_st;                 /* number of texture coordinates */
    int32_t num_tris;               /* number of triangles */
    int32_t num_glcmds;             /* number of opengl commands */
    int32_t num_frames;             /* number of frames */

    int32_t offset_skins;           /* offset skin data */
    int32_t offset_st;              /* offset texture coordinate data */
    int32_t offset_tris;            /* offset triangle data */
    int32_t offset_frames;          /* offset frame data */
    int32_t offset_glcmds;          /* offset OpenGL command data */
    int32_t offset_end;             /* offset end of file */
};

struct MD2Skin {
    char name[64];
};

struct MD2TexCoord {
    int16_t s;
    int16_t t;
};

struct MD2Triangle {
    uint16_t index[3];
    uint16_t st[3];
};

struct MD2Vertex {
    uint8_t v[3];
    uint8_t normal;
};

static Vec3 ANORMS [] = {
    #include "md2_anorms.h"
};


uint16_t MD2Loader::MAX_RESIDENT_FRAMES = 32;

class MD2MeshFrameData : public FrameUnpacker {
    /*
     * This stores the compressed MD2 mesh data as stored in the file. At any one time we
     * have up to MD2Loader::MAX_RESIDENT_FRAMES (default 32) uncompressed in memory. This allows balancing
     * performance vs memory. Turn the value up to improve performance but use more ram.
     */
public:
    /* This is the frame data from an MD2 file, but without the vertices */
    struct FrameTransform {
        Vec3 scale;
        Vec3 translate;
    };

    struct FrameVertex {
        uint8_t v[3];
        Vec2 st;
        uint8_t normal;
    };

    uint16_t vertex_count = 0;

    /* This contains all the vertices for all frames sequentially */
    std::vector<FrameVertex> vertices_;

    /* This contains the scale/translate data for each frame */
    std::vector<FrameTransform> frames_;

    struct UnpackedVertex {
        Vec3 v;
        Vec3 n;
    };

    typedef std::vector<UnpackedVertex> UnpackedFrame;

    /* Cache of recently used frames, trying to balance memory usage with
     * performance */
    std::unordered_map<uint16_t, UnpackedFrame> frame_cache;
    std::unordered_map<uint16_t, uint64_t> frame_usage_times;

    void _expand_verts(uint16_t frame) {
        /* Decompresses a single frame of MD2 data into the frame cache */

        static const Mat4 ROT_X = Mat4::as_rotation_x(Degrees(90.0f));
        static const Mat4 ROT_Y = Mat4::as_rotation_y(Degrees(90.0f));
        static const Mat4 VERTEX_ROTATION = ROT_Y * ROT_X;

        if(frame_cache.count(frame)) {
            frame_usage_times[frame] = TimeKeeper::now_in_us();
        } else {
            if(frame_cache.size() == MD2Loader::MAX_RESIDENT_FRAMES) {
                /* We need to clear out the oldest frame */

                uint64_t oldest_time = std::numeric_limits<uint64_t>::max();
                uint16_t oldest_frame = std::numeric_limits<uint16_t>::max();

                for(auto it = frame_usage_times.begin(); it != frame_usage_times.end(); ++it) {
                    if(it->second < oldest_time) {
                        oldest_time = it->second;
                        oldest_frame = it->first;
                    }
                }

                frame_cache.erase(oldest_frame);
                frame_usage_times.erase(oldest_frame);
            }

            auto& verts = frame_cache[frame];
            frame_usage_times[frame] = TimeKeeper::now_in_us();

            FrameTransform& frame1 = frames_[frame];
            FrameVertex* v1 = &vertices_[vertex_count * frame];

            verts.resize(vertex_count);
            for(uint16_t i = 0; i < vertex_count; ++i) {
                float vx1 = float(v1->v[0]) * frame1.scale.x + frame1.translate.x;
                float vy1 = float(v1->v[1]) * frame1.scale.y + frame1.translate.y;
                float vz1 = float(v1->v[2]) * frame1.scale.z + frame1.translate.z;

                verts[i].v = Vec3(vx1, vy1, vz1).rotated_by(VERTEX_ROTATION);
                verts[i].n = ANORMS[v1->normal].rotated_by(VERTEX_ROTATION);

                v1++;
            }
        }
    }

    void prepare_unpack(uint32_t, uint32_t, float, Rig* const, Debug* const = nullptr) override {
        // INTENTIONALLY BLANK
    }

    void unpack_frame(
      const uint32_t current_frame,
      const uint32_t next_frame,
      const float t,
      Rig* const rig,
      VertexData* const out,
      Debug* const debug=nullptr
    ) {
        _S_UNUSED(rig);
        _S_UNUSED(debug);  // We don't have any debugging for MD2 models. Maybe normals?

        _expand_verts(current_frame);
        _expand_verts(next_frame);

        FrameVertex* v1 = &vertices_[vertex_count * current_frame];
        FrameVertex* v2 = &vertices_[vertex_count * next_frame];

        out->resize(vertex_count);
        out->move_to_start();

        const auto& current_cache = frame_cache[current_frame];
        const auto& next_cache = frame_cache[next_frame];

        for(uint16_t i = 0; i < vertex_count; ++i) {
            const auto& v1v = current_cache[i].v;
            const auto& v2v = next_cache[i].v;
            const auto& n1 = current_cache[i].n;
            const auto& n2 = next_cache[i].n;

            out->position(v1v + (v2v - v1v) * t);
            out->tex_coord0(v1->st);
            out->diffuse(smlt::Colour::WHITE);
            out->normal(n1 + (n2 - n1) * t);
            out->move_next();

            ++v1;
            ++v2;
        }

        out->done();
    }
};

typedef std::shared_ptr<MD2MeshFrameData> MD2MeshFrameDataPtr;


const int32_t MAGIC_NUMBER_ID = 844121161;

void MD2Loader::into(Loadable &resource, const LoaderOptions &options) {
    _S_UNUSED(options);

    Mesh* mesh = loadable_to<Mesh>(resource);
    AssetManager* asset_manager = &mesh->asset_manager();

    assert(mesh && "Tried to load an MD2 file into something that wasn't a mesh");

    VertexSpecification vertex_specification = VertexSpecification::DEFAULT;

    // Rebuild the mesh from the loaded data
    mesh->reset(vertex_specification);

    auto mat = asset_manager->clone_default_material();

    SubMesh* submesh = mesh->new_submesh("default", mat, INDEX_TYPE_16_BIT, MESH_ARRANGEMENT_TRIANGLES);

    S_DEBUG("Loading MD2 model: {0}", filename_);

    MD2Header header;

    data_->read((char*) &header, sizeof(MD2Header));

    if(header.ident != MAGIC_NUMBER_ID || header.version != 8) {
        throw std::logic_error("Unsupported MD2 file: " + this->filename_.str());
    }

    data_->seekg(header.offset_frames, std::ios_base::beg);

    std::vector<std::vector<MD2Vertex>> vertices_by_frame;

    MD2MeshFrameDataPtr frame_data = std::make_shared<MD2MeshFrameData>();

    /* Load all the vertex data from the frames */
    for(auto i = 0; i < header.num_frames; ++i) {
        MD2MeshFrameData::FrameTransform frame_transform;
        char name[16];


        data_->read((char*) &frame_transform.scale, sizeof(Vec3));
        data_->read((char*) &frame_transform.translate, sizeof(Vec3));
        data_->read(name, sizeof(char) * 16);

        std::vector<MD2Vertex> frame_vertices(header.num_vertices);

        data_->read((char*) &frame_vertices[0], sizeof(MD2Vertex) * header.num_vertices);

        vertices_by_frame.push_back(frame_vertices);

        frame_data->frames_.push_back(frame_transform);
    }

    //========== LOAD SKINS ===================

    std::vector<MD2Skin> skins(header.num_skins);
    data_->seekg(header.offset_skins, std::ios::beg);
    data_->read((char*) &skins[0], sizeof(MD2Skin) * header.num_skins);

    MD2Skin* skin = &skins[0];

    // Only load the first skin
    std::string skin_name(skin->name);
    skin_name = kfs::path::norm_path(skin->name);

    std::vector<std::string> possible_paths = {
        kfs::path::join(kfs::path::dir_name(filename_.str()), kfs::path::split(skin_name).second),
        skin_name
    };

    smlt::TextureID tex_id;
    bool found = false;
    for(auto& texture_path: possible_paths) {
        auto p = vfs->locate_file(texture_path);
        if(p.has_value()) {
            tex_id = asset_manager->new_texture_from_file(p.value());
            found = true;
            break;
        } else {
            S_DEBUG("MD2 skin not found at: {0}", texture_path);
        }
    }

    if(!found) {
        S_WARN("Unable to locate MD2 skin: {0}", skin_name);
    }

    auto material = asset_manager->clone_default_material();
    material->set_diffuse_map(asset_manager->texture(tex_id));

    submesh->set_material(material);

    // =========== TEXTURE COORDS =============
    std::vector<MD2TexCoord> texture_coordinates(header.num_st);
    data_->seekg(header.offset_st, std::ios_base::beg);
    data_->read((char*) &texture_coordinates[0], sizeof(MD2TexCoord) * header.num_st);

    // =========== TRIANGLES ==================
    std::vector<MD2Triangle> triangles(header.num_tris);
    data_->seekg(header.offset_tris, std::ios_base::beg);
    data_->read((char*) &triangles[0], sizeof(MD2Triangle) * header.num_tris);

    /* We have to combine MD2Vertices with the ST coordinates from the triangles
     * which means we end up with more vertices at the end. We look the triangles
     * twice, once to generate the vertices for the first frame (while counting how many we do)
     * and then again to generate the rest of the vertices for the other frames
     */

    typedef std::pair<uint16_t, uint16_t> VertexKey;

    uint16_t current_frame = 0;
    for(auto& frame_transform: frame_data->frames_) {
        _S_UNUSED(frame_transform);

        std::map<VertexKey, uint16_t> seen_vertices;

        for(auto& triangle: triangles) {
            for(uint8_t i = 0; i < 3; ++i) {
                MD2MeshFrameData::FrameVertex vert;

                VertexKey key = std::make_pair(triangle.index[i], triangle.st[i]);

                if(seen_vertices.count(key)) {
                    if(current_frame == 0) {
                        submesh->index_data->index(seen_vertices[key]);
                    }
                } else {
                    MD2Vertex& source_vert = vertices_by_frame[current_frame][triangle.index[i]];

                    vert.v[0] = source_vert.v[0];
                    vert.v[1] = source_vert.v[1];
                    vert.v[2] = source_vert.v[2];
                    vert.normal = source_vert.normal;
                    vert.st = Vec2(
                        float(texture_coordinates[triangle.st[i]].s),
                        float(texture_coordinates[triangle.st[i]].t)
                    );

                    vert.st.x /= float(header.skinwidth);
                    vert.st.y /= float(header.skinheight);
                    vert.st.y *= -1.0f;

                    if(current_frame == 0) {
                        submesh->index_data->index(frame_data->vertices_.size());
                    }

                    seen_vertices.insert(std::make_pair(key, frame_data->vertices_.size()));
                    frame_data->vertices_.push_back(vert);
                }
            }
        }

        if(current_frame == 0) {
            // After the first frame, we know how many verts there are per-frame
            frame_data->vertex_count = frame_data->vertices_.size();
        }

        ++current_frame;
    }

    S_DEBUG("Loaded MD2 data, converting to mesh");

    mesh->enable_animation(MESH_ANIMATION_TYPE_VERTEX_MORPH, header.num_frames, frame_data);

    // This is necessary due to the difference in coordinate system
    submesh->reverse_winding();

    submesh->index_data->done();

    /*
     *     {   0,  39,  9 },   // STAND
    {  40,  45, 10 },   // RUN
    {  46,  53, 10 },   // ATTACK
    {  54,  57,  7 },   // PAIN_A
    {  58,  61,  7 },   // PAIN_B
    {  62,  65,  7 },   // PAIN_C
    {  66,  71,  7 },   // JUMP
    {  72,  83,  7 },   // FLIP
    {  84,  94,  7 },   // SALUTE
    {  95, 111, 10 },   // FALLBACK
    { 112, 122,  7 },   // WAVE
    { 123, 134,  6 },   // POINT
    { 135, 153, 10 },   // CROUCH_STAND
    { 154, 159,  7 },   // CROUCH_WALK
    { 160, 168, 10 },   // CROUCH_ATTACK
    { 196, 172,  7 },   // CROUCH_PAIN
    { 173, 177,  5 },   // CROUCH_DEATH
    { 178, 183,  7 },   // DEATH_FALLBACK
    { 184, 189,  7 },   // DEATH_FALLFORWARD
    { 190, 197,  7 },   // DEATH_FALLBACKSLOW
    { 198, 198,  5 },   // BOOM
    */
    mesh->add_animation("idle_1", 0, 39, 9.0);
    mesh->add_animation("running", 40, 45, 10.0);
    mesh->add_animation("attack", 46, 53, 10.0);
    mesh->add_animation("pain_1", 54, 57, 7.0);
    mesh->add_animation("pain_2", 58, 61, 7.0);
    mesh->add_animation("pain_3", 62, 65, 7.0);
    mesh->add_animation("jumping", 66, 71, 7.0);
    mesh->add_animation("taunt_1", 72, 83, 7.0);
    mesh->add_animation("taunt_2", 84, 94, 7.0);
    mesh->add_animation("fall_back", 95, 111, 10.0);
    mesh->add_animation("idle_2", 112, 122, 7.0);
    mesh->add_animation("idle_3", 123, 134, 6.0);
    mesh->add_animation("crouch_idle", 135, 153, 10.0);
    mesh->add_animation("crouch_walk", 154, 159, 7.0);
    mesh->add_animation("crouch_attack", 160, 168, 10.0);
    mesh->add_animation("crouch_pain", 169, 172, 7.0);
    mesh->add_animation("crouch_death", 173, 177, 5.0);
    mesh->add_animation("death_1", 178, 183, 7.0);
    mesh->add_animation("death_2", 184, 189, 7.0);
    mesh->add_animation("death_3", 190, 197, 7.0);
    mesh->add_animation("death_4", 198, 198, 5.0);

    S_DEBUG("Done loading MD2");
}


} //loaders
} //smlt
