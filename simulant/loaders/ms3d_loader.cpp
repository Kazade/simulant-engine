#include "ms3d_loader.h"

#include <cstring>
#include <map>
#include <vector>

#include "../application.h"
#include "../asset_manager.h"
#include "../assets/prefab.h"
#include "../generic/raii.h"
#include "../meshes/mesh.h"
#include "../nodes/animation_controller.h"
#include "../platform.h"
#include "../utils/pbr.h"
#include "../vertex_data.h"
#include "../vfs.h"
#include "../window.h"

namespace smlt {
namespace loaders {

struct MS3DHeader {
    char id[10];
    int32_t version;
};

struct MS3DVertex {
    uint8_t flags;
    smlt::Vec3 xyz;
    int8_t bone;
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
    smlt::Color ambient;
    smlt::Color diffuse;
    smlt::Color specular;
    smlt::Color emissive;
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

struct MS3DComment {
    int32_t index;
    int32_t comment_length;
    std::vector<char> comment;
};

struct MS3DVertexExtra {
    int8_t bone_ids[3];
    // 0 is the weight for the vertex bone, 1 for bone_ids[0] and 2 for bone_ids[1]
    // weight for bone_ids[2] is 1.0 - other_weights
    uint8_t weights[3];
    uint32_t extra; // Only if subversion is 2
};

/* An MS3D file holds a single unnamed timeline of keyframes. The generated
 * Prefab exposes it under this name (e.g. anim_controller->play("default")) */
const char* MS3D_ANIMATION_NAME = "default";

/* The Prefab node id given to the Armature holding the generated mesh (or a
 * plain Actor, if the file has no skeleton). Joints follow on from it -
 * joint N is node id (N + 1) */
const uint32_t MS3D_MESH_NODE_ID = 0;

/* MS3D name fields are fixed-size and are only NULL terminated if the name
 * is shorter than the field */
static std::string read_name(const char* field, std::size_t size) {
    return std::string(field, strnlen(field, size));
}

static Quaternion euler_to_quaternion(const Vec3& angles) {
    return Quaternion(Radians(angles.x), Radians(angles.y), Radians(angles.z));
}

MS3DLoader::MS3DLoader(const Path& filename, std::shared_ptr<std::istream> data):
    Loader(filename, data) {}

bool MS3DLoader::into(Loadable& resource, const LoaderOptions& options) {
    MeshLoadOptions mesh_opts;
    auto opts_it = options.find(MESH_LOAD_OPTIONS_KEY);
    if(opts_it != options.end()) {
        mesh_opts = smlt::any_cast<MeshLoadOptions>(opts_it->second);
    }

    bool use_asset_cache = mesh_opts.use_asset_cache;
    if(options.count("use_asset_cache")) {
        use_asset_cache = any_cast<bool>(options.at("use_asset_cache"));
    }

    S_DEBUG("MS3D: Beginning read..");

    Prefab* prefab = loadable_to<Prefab>(resource);
    if(!prefab) {
        S_ERROR("MS3D files must be loaded into a Prefab (see load_prefab)");
        return false;
    }

    AssetManager* assets = &prefab->asset_manager();

    MS3DHeader header;
    data_->read((char*) &header.id, sizeof(char) * 10);
    data_->read((char*) &header.version, sizeof(int32_t));

    std::string header_id(header.id, 10);

    if(header_id != "MS3D000000") {
        S_ERROR("Unsupported MS3D file. ID mismatch");
        return false;
    }

    S_DEBUG("MS3D: Header OK.");

    uint16_t num_vertices = 0;
    uint16_t num_triangles = 0;
    uint16_t num_groups = 0;
    uint16_t num_materials = 0;
    uint16_t num_joints = 0;

    int32_t comment_subversion = 0;
    int32_t num_group_comments = 0;
    int32_t num_material_comments = 0;
    int32_t num_joint_comments = 0;
    int32_t has_model_comment = 0;
    int32_t vertex_extra_subversion = 0;

    data_->read((char*) &num_vertices, sizeof(uint16_t));

    S_DEBUG("MS3D: Reading vertices...");

    std::vector<MS3DVertex> vertices(num_vertices);
    for(auto& vert: vertices) {
        data_->read((char*) &vert.flags, sizeof(MS3DVertex::flags));
        data_->read((char*) &vert.xyz, sizeof(MS3DVertex::xyz));
        data_->read((char*) &vert.bone, sizeof(MS3DVertex::bone));
        data_->read((char*) &vert.ref_count, sizeof(MS3DVertex::ref_count));
    }

    S_DEBUG("MS3D: Reading triangles...");

    data_->read((char*) &num_triangles, sizeof(uint16_t));
    std::vector<MS3DTriangle> triangles(num_triangles);
    for(auto& tri: triangles) {
        data_->read((char*) &tri.flags, sizeof(MS3DTriangle::flags));
        data_->read((char*) &tri.indices, sizeof(MS3DTriangle::indices));
        data_->read((char*) &tri.normals, sizeof(MS3DTriangle::normals));
        data_->read((char*) &tri.s, sizeof(MS3DTriangle::s));
        data_->read((char*) &tri.t, sizeof(MS3DTriangle::t));
        data_->read((char*) &tri.smoothingGroup, sizeof(MS3DTriangle::smoothingGroup));
        data_->read((char*) &tri.groupIndex, sizeof(MS3DTriangle::groupIndex));
    }

    S_DEBUG("MS3D: Reading groups...");

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

    S_DEBUG("MS3D: Reading materials...");

    data_->read((char*) &num_materials, sizeof(uint16_t));
    std::vector<MS3DMaterial> materials(num_materials);
    for(auto& mat: materials) {
        data_->read((char*) &mat.name, sizeof(MS3DMaterial::name));
        data_->read((char*) &mat.ambient, sizeof(MS3DMaterial::ambient));
        data_->read((char*) &mat.diffuse, sizeof(MS3DMaterial::diffuse));
        data_->read((char*) &mat.specular, sizeof(MS3DMaterial::specular));
        data_->read((char*) &mat.emissive, sizeof(MS3DMaterial::emissive));
        data_->read((char*) &mat.shininess, sizeof(MS3DMaterial::shininess));
        data_->read((char*) &mat.transparency, sizeof(MS3DMaterial::transparency));
        data_->read((char*) &mat.mode, sizeof(MS3DMaterial::mode));
        data_->read((char*) &mat.texture, sizeof(MS3DMaterial::texture));
        data_->read((char*) &mat.alphamap, sizeof(MS3DMaterial::alphamap));
    }

    MS3DAnimData anim_data;
    data_->read((char*) &anim_data, sizeof(MS3DAnimData));
    data_->read((char*) &num_joints, sizeof(uint16_t));

    S_DEBUG("MS3D: Reading joints...");

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

        for(uint16_t j = 0; j < joints[i].num_rotation_key_frames; ++j) {
            data_->read((char*) &joints[i].rotation_key_frames[j], sizeof(MS3DRotationKeyFrame));
        }

        for(uint16_t j = 0; j < joints[i].num_position_key_frames; ++j) {
            data_->read((char*) &joints[i].position_key_frames[j], sizeof(MS3DPositionKeyFrame));
        }
    }

    std::vector<MS3DVertexExtra> vertex_extras;

    /* Try to read the comment subversion, we might then trip over the
     * end of the file if this is a V0 file */
    data_->read((char*) &comment_subversion, sizeof(int32_t));

    if(data_->eof()) {
        vertex_extra_subversion = 0;
    } else {
        S_DEBUG("MS3D: Reading comments...");

        MS3DComment comment; // We do nothing with this for now

        data_->read((char*) &num_group_comments, sizeof(int32_t));
        for(int32_t i = 0; i < num_group_comments; ++i) {
            data_->read((char*) &comment.index, sizeof(comment.index));
            data_->read((char*) &comment.comment_length, sizeof(comment.comment_length));
            comment.comment.resize(comment.comment_length);
            data_->read((char*) &comment.comment[0], sizeof(char) * comment.comment_length);
        }

        data_->read((char*) &num_material_comments, sizeof(int32_t));
        for(int32_t i = 0; i < num_material_comments; ++i) {
            data_->read((char*) &comment.index, sizeof(comment.index));
            data_->read((char*) &comment.comment_length, sizeof(comment.comment_length));
            comment.comment.resize(comment.comment_length);
            data_->read((char*) &comment.comment[0], sizeof(char) * comment.comment_length);
        }

        data_->read((char*) &num_joint_comments, sizeof(int32_t));
        for(int32_t i = 0; i < num_joint_comments; ++i) {
            data_->read((char*) &comment.index, sizeof(comment.index));
            data_->read((char*) &comment.comment_length, sizeof(comment.comment_length));
            comment.comment.resize(comment.comment_length);
            data_->read((char*) &comment.comment[0], sizeof(char) * comment.comment_length);
        }

        data_->read((char*) &has_model_comment, sizeof(int32_t));
        for(int32_t i = 0; i < has_model_comment; ++i) {
            data_->read((char*) &comment.index, sizeof(comment.index));
            data_->read((char*) &comment.comment_length, sizeof(comment.comment_length));
            comment.comment.resize(comment.comment_length);
            data_->read((char*) &comment.comment[0], sizeof(char) * comment.comment_length);
        }

        data_->read((char*) &vertex_extra_subversion, sizeof(vertex_extra_subversion));

        vertex_extras.resize(num_vertices);
        for(uint16_t i = 0; i < num_vertices; ++i) {
            MS3DVertexExtra& extra = vertex_extras[i];
            data_->read((char*) extra.bone_ids, sizeof(uint8_t) * 3);
            data_->read((char*) extra.weights, sizeof(uint8_t) * 3);
            if(vertex_extra_subversion >= 2) {
                data_->read((char*) &extra.extra, sizeof(extra.extra));
            }
        }
    }

    /* Resolve the joint hierarchy. MS3D stores joints parent-first, but rather
     * than rely on that we order them ourselves so that a joint always follows
     * its parent - both the bind matrices below and Prefab::push_node need
     * that ordering */
    std::vector<int32_t> parent_of(joints.size(), -1);
    {
        std::map<std::string, std::size_t> joint_by_name;
        for(std::size_t i = 0; i < joints.size(); ++i) {
            joint_by_name[read_name(joints[i].name, 32)] = i;
        }

        for(std::size_t i = 0; i < joints.size(); ++i) {
            auto parent_name = read_name(joints[i].parent_name, 32);
            if(parent_name.empty()) {
                continue;
            }

            auto it = joint_by_name.find(parent_name);
            if(it == joint_by_name.end()) {
                S_WARN("MS3D: Joint {0} has an unknown parent ({1})",
                       read_name(joints[i].name, 32), parent_name);
                continue;
            }

            parent_of[i] = (int32_t) it->second;
        }
    }

    std::vector<std::size_t> joint_order;
    {
        joint_order.reserve(joints.size());

        std::vector<bool> placed(joints.size(), false);
        bool progressed = true;
        while(joint_order.size() < joints.size() && progressed) {
            progressed = false;
            for(std::size_t i = 0; i < joints.size(); ++i) {
                if(placed[i] || (parent_of[i] >= 0 && !placed[parent_of[i]])) {
                    continue;
                }

                placed[i] = true;
                joint_order.push_back(i);
                progressed = true;
            }
        }

        /* Anything left over is part of a cycle - detach it so we still
         * produce something usable */
        for(std::size_t i = 0; i < joints.size(); ++i) {
            if(!placed[i]) {
                S_WARN("MS3D: Joint {0} is part of a parenting cycle, detaching",
                       read_name(joints[i].name, 32));
                parent_of[i] = -1;
                joint_order.push_back(i);
            }
        }
    }

    /* The bind pose. `abs_rest` transforms a point from the joint's space into
     * mesh space, so its inverse is the inverse bind matrix used for skinning */
    std::vector<Mat4> abs_rest(joints.size());
    for(auto i: joint_order) {
        auto local = Mat4::as_transform(joints[i].position,
                                        euler_to_quaternion(joints[i].rotation),
                                        Vec3(1, 1, 1));

        abs_rest[i] = (parent_of[i] < 0) ? local : abs_rest[parent_of[i]] * local;
    }

    /* MS3D files reference their textures relative to the model, so make sure
     * the containing folder is searched while we load */
    auto folder = filename_.parent();
    auto added = get_app()->vfs->insert_search_path(0, folder);
    raii::Finally finally([&]() {
        if(added) {
            get_app()->vfs->remove_search_path(folder);
        }
    });

    S_DEBUG("MS3D: Generating mesh");

    /* Positions are duplicated per-triangle-corner (MS3D shares vertices
     * between faces with differing normals/UVs) hence the triangle count
     * rather than the vertex count here */
    auto spec = VertexSpecification(
        VERTEX_ATTRIBUTE_3F,   // Position
        VERTEX_ATTRIBUTE_3F,   // Normal
        VERTEX_ATTRIBUTE_2F,   // UV
        VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE,
        VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE, VERTEX_ATTRIBUTE_NONE,
        VERTEX_ATTRIBUTE_NONE,
        VERTEX_ATTRIBUTE_4F,   // Color
        VERTEX_ATTRIBUTE_NONE, // Specular
        joints.empty() ? VERTEX_ATTRIBUTE_NONE : VERTEX_ATTRIBUTE_4UB, // Joints
        joints.empty() ? VERTEX_ATTRIBUTE_NONE : VERTEX_ATTRIBUTE_4F   // Weights
    );

    auto mesh = assets->create_mesh(spec);
    mesh->set_name(filename_.name());

    auto vdata = mesh->vertex_data.get();

    std::map<std::string, TexturePtr> loaded_textures;
    std::map<int, MaterialPtr> loaded_materials;
    std::map<int, SubMeshPtr> submeshes;

    auto material_for_group = [&](int material_index) -> MaterialPtr {
        auto existing = loaded_materials.find(material_index);
        if(existing != loaded_materials.end()) {
            return existing->second;
        }

        smlt::MaterialPtr mat = assets->create_material();
        mat->set_lighting_enabled(true);

        /* Groups without a material just get the default one */
        if(material_index < 0 || material_index >= (int) materials.size()) {
            mat->set_textures_enabled(0);
            loaded_materials[material_index] = mat;
            return mat;
        }

        auto& material = materials[material_index];

        auto s = traditional_to_pbr(material.ambient, material.diffuse,
                                    material.specular, material.shininess);

        mat->set_metallic(s.metallic);
        mat->set_roughness(s.roughness);
        mat->set_base_color(s.base_color);
        mat->set_name(read_name(material.name, 32));

        /* norm_path("") is ".", so only normalise once we know there's
         * actually a texture to look for */
        auto texname = read_name(material.texture, 128);
        if(!texname.empty()) {
            texname = kfs::path::norm_path(texname);
            if(texname.size() > 1 && texname[0] == '.' && texname[1] == '\\') {
                texname = texname.substr(2);
            }

            S_DEBUG("MS3D: Loading texture {0}...", texname);

            TextureFlags tex_flags;
            tex_flags.use_asset_cache = use_asset_cache;

            auto tex = (loaded_textures.count(texname))
                           ? loaded_textures[texname]
                           : assets->load_texture(texname, tex_flags);

            if(!tex) {
                /* Sometimes MS3D files use absolute paths which is no good
                 * so if the texture isn't found, fallback to looking in the
                 * current directory */
                std::replace(texname.begin(), texname.end(), '\\', kfs::SEP[0]);
                Path fallback = kfs::path::split(texname).second;
                tex = assets->load_texture(fallback, tex_flags);
            }

            if(tex) {
                loaded_textures[texname] = tex;
                mat->set_base_color_map(tex);
                mat->set_textures_enabled(BASE_COLOR_MAP_ENABLED);
                prefab->push_texture(tex);
            }
        }

        loaded_materials[material_index] = mat;
        prefab->push_material(mat);
        return mat;
    };

    for(auto& group: groups) {
        int material_index = group.material_index;
        auto mat = material_for_group(material_index);

        /* Groups sharing a material share a submesh */
        SubMeshPtr sm;
        auto existing = submeshes.find(material_index);
        if(existing != submeshes.end()) {
            sm = existing->second;
        } else {
            auto sm_name = mat->name();
            if(sm_name.empty() || mesh->has_submesh(sm_name)) {
                sm_name = _F("Material {0}").format(material_index);
            }

            sm = mesh->create_submesh(sm_name, mat, INDEX_TYPE_16_BIT);
            submeshes[material_index] = sm;
        }

        auto idata = sm->index_data.get();

        for(auto idx: group.triangle_indices) {
            auto& triangle = triangles[idx];
            for(int i = 0; i < 3; ++i) {
                auto vert_index = triangle.indices[i];
                vdata->position(vertices[vert_index].xyz);
                vdata->tex_coord0(triangle.s[i], 1.0f - triangle.t[i]);
                vdata->normal(triangle.normals[i]);
                vdata->color(Color::white());

                if(!joints.empty()) {
                    int8_t bones[4] = {
                        vertices[vert_index].bone,
                        (vertex_extra_subversion == 0) ? (int8_t) -1 : vertex_extras[vert_index].bone_ids[0],
                        (vertex_extra_subversion == 0) ? (int8_t) -1 : vertex_extras[vert_index].bone_ids[1],
                        (vertex_extra_subversion == 0) ? (int8_t) -1 : vertex_extras[vert_index].bone_ids[2],
                    };

                    uint8_t weights[4] = {
                        (bones[0] > -1 && vertex_extra_subversion > 0) ? vertex_extras[vert_index].weights[0] : (uint8_t) 0,
                        (bones[1] > -1 && vertex_extra_subversion > 0) ? vertex_extras[vert_index].weights[1] : (uint8_t) 0,
                        (bones[2] > -1 && vertex_extra_subversion > 0) ? vertex_extras[vert_index].weights[2] : (uint8_t) 0,
                        0
                    };

                    int range = (vertex_extra_subversion <= 1) ? 255 : 100;

                    weights[3] = (bones[3] > -1) ?
                        range - weights[2] - weights[1] - weights[0] : 0;

                    if(weights[0] + weights[1] + weights[2] + weights[3] == 0) {
                        weights[0] = (bones[0] > -1) ? range : 0;
                    }

                    float weight_scalar = 1.0f / float(range);

                    uint8_t out_joints[4] = {0, 0, 0, 0};
                    float out_weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

                    for(uint8_t k = 0; k < 4; ++k) {
                        int8_t bone = bones[k];
                        if(bone > -1 && (std::size_t) bone < joints.size()) {
                            out_joints[k] = (uint8_t) bone;
                            out_weights[k] = float(weights[k]) * weight_scalar;
                        }
                    }

                    vdata->joints<uint8_t>(out_joints[0], out_joints[1],
                                           out_joints[2], out_joints[3]);
                    vdata->weights<float>(out_weights[0], out_weights[1],
                                          out_weights[2], out_weights[3]);
                }

                vdata->move_next();

                idata->index(vdata->count() - 1);
            }
        }

        idata->done();
    }

    vdata->done();

    if(!joints.empty()) {
        auto skin = std::make_shared<Mesh::Skin>();
        skin->inverse_bind_matrices.reserve(joints.size());

        for(std::size_t i = 0; i < joints.size(); ++i) {
            skin->inverse_bind_matrices.push_back(abs_rest[i].inversed());
        }

        mesh->skin = skin;
    }

    prefab->push_mesh(mesh);

    /* Now build the node hierarchy. MS3D joint transforms are relative to
     * the model origin, so the Armature holding the mesh sits there and the
     * root joints hang directly off it. */

    PrefabNode mesh_node;
    mesh_node.id = MS3D_MESH_NODE_ID;
    mesh_node.node_type_name = joints.empty() ? "actor" : "armature";
    mesh_node.name = mesh->name();
    mesh_node.params.set("mesh", mesh);
    mesh_node.params.set("translation", Vec3());
    mesh_node.params.set("rotation", Quaternion());
    mesh_node.params.set("scale_factor", Vec3(1, 1, 1));
    prefab->push_node(mesh_node, -1);

    std::vector<PrefabNode> joint_nodes(joints.size());
    for(auto i: joint_order) {
        PrefabNode& node = joint_nodes[i];
        node.id = (uint32_t) (i + 1);
        node.node_type_name = "joint";
        node.name = read_name(joints[i].name, 32);
        node.params.set("joint_index", (int) i);
        node.params.set("translation", joints[i].position);
        node.params.set("rotation", euler_to_quaternion(joints[i].rotation));
        node.params.set("scale_factor", Vec3(1, 1, 1));

        int32_t parent_node_id = (parent_of[i] < 0)
                                     ? (int32_t)MS3D_MESH_NODE_ID
                                     : (int32_t)(parent_of[i] + 1);

        prefab->push_node(node, parent_node_id);

        S_DEBUG("Loaded joint {0}", node.name.str());
    }

    /* Finally the animation. MS3D keyframes are stored per-joint, sparsely,
     * and relative to the joint's rest pose - which maps directly onto the
     * translation/rotation channels of a Prefab animation */

    float duration = 0.0f;
    if(anim_data.fps > 0.0f) {
        duration = float(anim_data.total_frames) / anim_data.fps;
    }

    for(auto& joint: joints) {
        for(auto& key: joint.rotation_key_frames) {
            duration = std::max(duration, key.time);
        }
        for(auto& key: joint.position_key_frames) {
            duration = std::max(duration, key.time);
        }
    }

    /* AnimationData interpolates between a pair of keys, so a channel with a
     * single key needs a second (identical) one to hold its value */
    auto pad_times = [&](std::vector<float>& times) {
        times.push_back((duration > times[0]) ? duration : times[0] + 1.0f);
    };

    for(std::size_t i = 0; i < joints.size(); ++i) {
        auto& joint = joints[i];

        if(!joint.position_key_frames.empty()) {
            std::vector<float> times;
            std::vector<Vec3> values;
            for(auto& key: joint.position_key_frames) {
                times.push_back(key.time);
                values.push_back(joint.position + key.position);
            }

            if(times.size() == 1) {
                pad_times(times);
                values.push_back(values[0]);
            }

            prefab->push_animation_channel(
                MS3D_ANIMATION_NAME, joint_nodes[i],
                ANIMATION_PATH_TRANSLATION, ANIMATION_INTERPOLATION_LINEAR,
                std::make_shared<AnimationData>(times, std::move(values)));
        }

        if(!joint.rotation_key_frames.empty()) {
            auto rest = euler_to_quaternion(joint.rotation);

            std::vector<float> times;
            std::vector<Quaternion> values;
            for(auto& key: joint.rotation_key_frames) {
                times.push_back(key.time);
                values.push_back(rest * euler_to_quaternion(key.rotation));
            }

            if(times.size() == 1) {
                pad_times(times);
                values.push_back(values[0]);
            }

            prefab->push_animation_channel(
                MS3D_ANIMATION_NAME, joint_nodes[i], ANIMATION_PATH_ROTATION,
                ANIMATION_INTERPOLATION_LINEAR,
                std::make_shared<AnimationData>(times, std::move(values)));
        }
    }

    return true;
}
}
}
