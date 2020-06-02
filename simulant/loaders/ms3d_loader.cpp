#include "ms3d_loader.h"
#include "../vertex_data.h"
#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "../vfs.h"
#include "../window.h"

namespace smlt {
namespace loaders {

#pragma pack(push, 1)
struct MS3DHeader {
    char id[10];
    int32_t version;
};

struct MS3DVertex {
    uint8_t flags;
    smlt::Vec3 xyz;
    uint8_t bone;
    uint8_t ref_count;
};

struct MS3DTriangle {
    uint16_t flags;
    uint16_t indices[3];
    smlt::Vec3 normals[3];
    float s[3];
    float t[3];
    uint8_t smoothingGroup;
    uint8_t groupIndex;
};

struct MS3DGroup {
    uint8_t flags;                              // SELECTED | HIDDEN
    char name[32];                           //
    uint16_t num_triangles;                       //
    std::vector<uint16_t> triangle_indices;      // the groups group the triangles
    char material_index;                      // -1 = no material
};

struct MS3DMaterial {
    char name[32];
    smlt::Colour ambient;
    smlt::Colour diffuse;
    smlt::Colour specular;
    smlt::Colour emissive;
    float shininess;  // 0.0f - 128.0f
    float transparency;   // 0.0f - 1.0f
    uint8_t mode;  // 0, 1, 2 is unused now
    char texture[128];  // texture.bmp
    char alphamap[128];  // alpha.bmp
};

struct MS3DAnimData {
    float fps;
    float current_time;
    int32_t total_frames;
};

struct MS3DRotationKeyFrame {
    float time;
    smlt::Vec3 rotation;
};

struct MS3DPositionKeyFrame {
    float time;
    smlt::Vec3 position;
};

struct MS3DJoint {
    uint8_t flags;                              // SELECTED | DIRTY
    char name[32];                           //
    char parent_name[32];                     //
    smlt::Vec3 rotation;                        // local reference matrix
    smlt::Vec3 position;

    uint16_t num_rotation_key_frames;
    uint16_t num_position_key_frames;

    std::vector<MS3DRotationKeyFrame> rotation_key_frames;
    std::vector<MS3DPositionKeyFrame> position_key_frames;
};

#pragma pack(pop)


MS3DLoader::MS3DLoader(const unicode& filename, std::shared_ptr<std::istream> data):
    Loader(filename, data) {}

void MS3DLoader::into(Loadable& resource, const LoaderOptions& options) {
    Mesh* mesh = loadable_to<Mesh>(resource);
    assert(mesh && "Tried to load an MS3D file into a non-mesh");

    AssetManager* assets = &mesh->asset_manager();

    mesh->reset(VertexSpecification::DEFAULT);

    MS3DHeader header;
    data_->read((char*) &header.id, sizeof(char) * 10);
    data_->read((char*) &header.version, sizeof(int32_t));

    if(strncmp(header.id, "MS3D000000", 10) != 0) {
        throw std::logic_error("Unsupported MS3D file. ID mismatch");
    }

    uint16_t num_vertices;
    uint16_t num_triangles;
    uint16_t num_groups;
    uint16_t num_materials;
    uint16_t num_joints;

    data_->read((char*) &num_vertices, sizeof(uint16_t));
    std::vector<MS3DVertex> vertices(num_vertices);
    data_->read((char*) &vertices[0], sizeof(MS3DVertex) * num_vertices);

    data_->read((char*) &num_triangles, sizeof(uint16_t));
    std::vector<MS3DTriangle> triangles(num_triangles);
    data_->read((char*) &triangles[0], sizeof(MS3DTriangle) * num_triangles);

    data_->read((char*) &num_groups, sizeof(uint16_t));
    std::vector<MS3DGroup> groups(num_groups);
    for(uint16_t i = 0; i < num_groups; ++i) {
        data_->read((char*) &groups[i].flags, sizeof(uint8_t));
        data_->read((char*) &groups[i].name, sizeof(char) * 32);
        data_->read((char*) &groups[i].num_triangles, sizeof(uint16_t));

        groups[i].triangle_indices.resize(groups[i].num_triangles);

        data_->read((char*) &groups[i].triangle_indices[0], sizeof(uint16_t) * groups[i].num_triangles);
        data_->read((char*) &groups[i].material_index, sizeof(char));
    }

    data_->read((char*) &num_materials, sizeof(uint16_t));
    std::vector<MS3DMaterial> materials(num_materials);
    data_->read((char*) &materials[0], sizeof(MS3DMaterial) * num_materials);

    MS3DAnimData anim_data;
    data_->read((char*) &anim_data, sizeof(MS3DAnimData));
    data_->read((char*) &num_joints, sizeof(uint16_t));

    std::vector<MS3DJoint> joints(num_joints);
    for(uint16_t i = 0; i < num_joints; ++i) {
        data_->read((char*) &joints[i].flags, sizeof(uint8_t));
        data_->read((char*) &joints[i].name, sizeof(char) * 32);
        data_->read((char*) &joints[i].parent_name, sizeof(char) * 32);
        data_->read((char*) &joints[i].rotation, sizeof(float) * 3);
        data_->read((char*) &joints[i].position, sizeof(float) * 3);
        data_->read((char*) &joints[i].num_rotation_key_frames, sizeof(uint16_t));
        data_->read((char*) &joints[i].num_position_key_frames, sizeof(uint16_t));

        joints[i].rotation_key_frames.resize(joints[i].num_rotation_key_frames);
        joints[i].position_key_frames.resize(joints[i].num_position_key_frames);

        data_->read(
            (char*) &joints[i].rotation_key_frames[0],
            sizeof(MS3DRotationKeyFrame) * joints[i].num_rotation_key_frames
        );

        data_->read(
            (char*) &joints[i].position_key_frames[0],
            sizeof(MS3DPositionKeyFrame) * joints[i].num_position_key_frames
        );
    }

    auto dir = kfs::path::dir_name(filename_.encode());
    bool remove_path = assets->window->vfs->add_search_path(dir);

    auto vdata = mesh->vertex_data.get();

    for(auto& group: groups) {
        auto& material = materials[group.material_index];
        smlt::MaterialPtr mat = assets->new_material();

        mat->set_ambient(material.ambient);
        mat->set_diffuse(material.diffuse);
        mat->set_specular(material.specular);
        mat->set_emission(material.emissive);
        mat->set_shininess(material.shininess);

        auto texname = kfs::path::norm_path(material.texture);
        if(texname[0] == '.' && texname[1] == '\\') {
            texname = texname.substr(2);
        }

        auto tex = assets->new_texture_from_file(texname);
        mat->set_diffuse_map(tex);

        mat->set_texturing_enabled(true);
        mat->set_lighting_enabled(true);

        auto sm = mesh->new_submesh_with_material(material.name, mat);
        auto idata = sm->index_data.get();

        for(auto idx: group.triangle_indices) {
            auto& triangle = triangles[idx];
            for(int i = 0; i < 3; ++i) {
                vdata->position(vertices[triangle.indices[i]].xyz);
                vdata->tex_coord0(triangle.s[i], triangle.t[i]);
                vdata->normal(triangle.normals[i]);
                vdata->diffuse(Colour::WHITE);
                vdata->move_next();

                idata->index(vdata->count() - 1);
            }
        }

        idata->done();
    }

    vdata->done();

    if(remove_path) {
        assets->window->vfs->remove_search_path(dir);
    }

    //mesh->enable_animation(MESH_ANIMATION_TYPE_SKELETAL, anim_data.total_frames, nullptr);
}

}
}
