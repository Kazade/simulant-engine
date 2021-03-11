#include "ms3d_loader.h"
#include "../vertex_data.h"
#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "../vfs.h"
#include "../window.h"
#include "../assets/meshes/skeleton.h"

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

MS3DLoader::MS3DLoader(const unicode& filename, std::shared_ptr<std::istream> data):
    Loader(filename, data) {}

void MS3DLoader::into(Loadable& resource, const LoaderOptions& options) {
    _S_UNUSED(options);

    S_DEBUG("MS3D: Beginning read..");

    Mesh* mesh = loadable_to<Mesh>(resource);
    assert(mesh && "Tried to load an MS3D file into a non-mesh");

    AssetManager* assets = &mesh->asset_manager();

    mesh->reset(VertexSpecification::DEFAULT);

    MS3DHeader header;
    data_->read((char*) &header.id, sizeof(char) * 10);
    data_->read((char*) &header.version, sizeof(int32_t));

    std::string header_id(header.id, 10);

    if(header_id != "MS3D000000") {
        throw std::logic_error("Unsupported MS3D file. ID mismatch");
    }

    S_DEBUG("MS3D: Header OK.");

    uint16_t num_vertices = 0;
    uint16_t num_triangles = 0;
    uint16_t num_groups = 0;
    uint16_t num_materials = 0;
    uint16_t num_joints = 0;

    int32_t comment_subversion = 0;
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

        data_->read(
            (char*) &joints[i].rotation_key_frames[0],
            sizeof(MS3DRotationKeyFrame) * joints[i].num_rotation_key_frames
        );

        data_->read(
            (char*) &joints[i].position_key_frames[0],
            sizeof(MS3DPositionKeyFrame) * joints[i].num_position_key_frames
        );
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
            if(vertex_extra_subversion == 2) {
                data_->read((char*) &extra.extra, sizeof(extra.extra));
            }
        }
    }

    /* Add a skeleton with the same number joints as
     * the MS3D model. Note, the number of vertices in the triangles does
     * not match as indexes share positions in the MS3D file and we need to
     * duplicate them in Simulant */
    mesh->add_skeleton(joints.size());

    /* Set the default fps to what's in the file */
    mesh->set_default_fps(anim_data.fps);

    auto frame_data = std::make_shared<SkeletalFrameUnpacker>(
        mesh,
        anim_data.total_frames,
        triangles.size() * 3  /* Vertex count */
    );

    auto dir = kfs::path::dir_name(filename_.encode());
    bool remove_path = assets->window->vfs->add_search_path(dir);

    auto vdata = mesh->vertex_data.get();

    S_DEBUG("MS3D: Generating mesh");

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

        S_DEBUG("MS3D: Loading texture {0}...", texname);

        auto tex = assets->new_texture_from_file(texname);
        if(tex) {
            mat->set_diffuse_map(tex);
        }

        mat->set_textures_enabled(1);
        mat->set_lighting_enabled(true);

        auto sm = mesh->new_submesh_with_material(material.name, mat);
        auto idata = sm->index_data.get();

        for(auto idx: group.triangle_indices) {
            auto& triangle = triangles[idx];
            for(int i = 0; i < 3; ++i) {
                auto vert_index = triangle.indices[i];
                vdata->position(vertices[vert_index].xyz);
                vdata->tex_coord0(triangle.s[i], 1.0f - triangle.t[i]);
                vdata->normal(triangle.normals[i]);
                vdata->diffuse(Colour::WHITE);
                vdata->move_next();

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
                    weights[0] = range;
                }

                float weight_scalar = 1.0f / float(range);

                for(uint8_t k = 0; k < 4; ++k) {
                    char bone = bones[k];
                    if(bone > -1) {
                        frame_data->link_vertex_to_joint(
                            vdata->count() - 1, bone, float(weights[k]) * weight_scalar
                        );
                    }
                }

                idata->index(vdata->count() - 1);
            }
        }

        idata->done();
    }

    vdata->done();

    if(remove_path) {
        assets->window->vfs->remove_search_path(dir);
    }

    auto to_quaternion = [](const Vec3& angles) -> Quaternion {
        /* This is what OGRE does... so it should be right! */

        auto qx = Quaternion(Vec3::POSITIVE_X, Radians(angles.x));
        auto qy = Quaternion(Vec3::POSITIVE_Y, Radians(angles.y));
        auto qz = Quaternion(Vec3::POSITIVE_Z, Radians(angles.z));

        return qz * qy * qx;
    };

    Skeleton* skeleton = mesh->skeleton;

    for(std::size_t i = 0; i < joints.size(); ++i) {
        auto joint_out = skeleton->joint(i);
        auto joint_in = &joints[i];

        joint_out->set_id(i);
        joint_out->set_name(std::string(joint_in->name));
        joint_out->move_to(joint_in->position);
        joint_out->rotate_to(to_quaternion(joint_in->rotation));

        S_DEBUG("Loaded joint {0}", joint_in->name);

        /* If we have a parent */
        if(joint_in->parent_name[0] != '\0') {
            /* Find the parent joint, and link it to this joint */
            auto parent_joint = skeleton->find_joint(joint_in->parent_name);
            assert(parent_joint);

            parent_joint->link_to(joint_out);
        }
    }

    /* MS3D files don't store entire joint keyframes of animation, they only
     * store the rotations or translations that changed. What we do here is
     * unpack them into our structure where we duplicate some state for better
     * performance during animation at the cost of a bit of memory */

    for(std::size_t i = 0; i < (std::size_t) anim_data.total_frames; ++i) {
        float frame_time = i / anim_data.fps;

        for(std::size_t j = 0; j < joints.size(); ++j) {
            JointState state;

            // FIXME: set state
            /* MS3D joints store key frames at the time in seconds
             * what we want to do is work out which keyframe is active
             * at the time of this frame */

            auto& source_joint = joints[j];

            /* Note: assumes keyframes are stored in order of time... */
            /* FIXME: Duplication of logic makes me sad. Maybe we can templatise this */
            MS3DRotationKeyFrame* last_rot_frame = nullptr;
            bool rot_frame_found = false;
            for(auto& kf: source_joint.rotation_key_frames) {
                if(kf.time > frame_time) {
                    /* OK, this frame is too late, choose the last one */
                    if(!last_rot_frame) {
                        /* We have to assume that this is the first frame, although
                         * this shouldn't happen, so let's log a warning */
                        S_WARN(
                            "First frame time was unexpectedly > 0, floating point issue?"
                        );
                        state.rotation = to_quaternion(source_joint.rotation_key_frames[0].rotation);
                    } else {
                        state.rotation = to_quaternion(last_rot_frame->rotation);
                    }

                    rot_frame_found = true;
                    break;
                }

                last_rot_frame = &kf;
            }

            if(!rot_frame_found) {
                if(source_joint.rotation_key_frames.empty()) {
                    state.rotation = Quaternion();
                } else {
                    state.rotation = to_quaternion(source_joint.rotation_key_frames.back().rotation);
                }
            }

            MS3DPositionKeyFrame* last_pos_frame = nullptr;
            bool pos_frame_found = false;
            for(auto& kf: source_joint.position_key_frames) {
                if(kf.time > frame_time) {
                    /* OK, this frame is too late, choose the last one */
                    if(!last_pos_frame) {
                        /* We have to assume that this is the first frame, although
                         * this shouldn't happen, so let's log a warning */
                        S_WARN(
                            "First frame time was unexpectedly > 0, floating point issue?"
                        );
                        state.translation = source_joint.position_key_frames[0].position;
                    } else {
                        state.translation = last_pos_frame->position;
                    }

                    pos_frame_found = true;
                    break;
                }

                last_pos_frame = &kf;
            }

            if(!pos_frame_found) {
                // If we didn't find anything, set it to the last position
                // or if there are no keyframes just set to the joint position
                if(source_joint.position_key_frames.empty()) {
                    state.translation = Vec3();
                } else {
                    state.translation = source_joint.position_key_frames.back().position;
                }
            }

            frame_data->set_joint_state_at_frame(
                i, j, state
            );
        }
    }

    /* Calculate the absolute transformations of all joints in
     * all keyframes */
    frame_data->rebuild_key_frame_absolute_transforms();

    mesh->enable_animation(
        MESH_ANIMATION_TYPE_SKELETAL,
        anim_data.total_frames,
        frame_data
    );
}

}
}
