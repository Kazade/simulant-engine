#pragma once

#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "simulant/test.h"

namespace {

using namespace smlt;

/* Builds a minimal but complete MS3D file in memory: a single triangle
 * skinned to a two joint skeleton, with a keyframed root joint.
 *
 * The file stops after the joint section, which makes it a "subversion 0"
 * file (no per-vertex extras) - the loader detects that by hitting EOF when
 * it tries to read the comment subversion. */
class MS3DBuilder {
public:
    void write(const void* data, std::size_t size) {
        auto at = reinterpret_cast<const uint8_t*>(data);
        bytes_.insert(bytes_.end(), at, at + size);
    }

    template<typename T>
    void write_pod(T value) {
        write(&value, sizeof(T));
    }

    void write_vec3(float x, float y, float z) {
        write_pod(x);
        write_pod(y);
        write_pod(z);
    }

    /* MS3D name fields are fixed size and zero padded */
    void write_name(const std::string& name, std::size_t size) {
        std::vector<char> field(size, '\0');
        std::memcpy(&field[0], name.c_str(), std::min(name.size(), size - 1));
        write(&field[0], size);
    }

    const std::vector<uint8_t>& bytes() const {
        return bytes_;
    }

private:
    std::vector<uint8_t> bytes_;
};

static std::vector<uint8_t> generate_ms3d() {
    MS3DBuilder b;

    b.write("MS3D000000", 10);
    b.write_pod<int32_t>(4); // version

    /* 3 vertices. The last one is deliberately unweighted (bone == -1) so
     * we exercise the "vertex with no joint" path */
    b.write_pod<uint16_t>(3);
    const int8_t vertex_bones[] = {0, 1, -1};
    const float vertex_positions[3][3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };

    for(int i = 0; i < 3; ++i) {
        b.write_pod<uint8_t>(0); // flags
        b.write_vec3(vertex_positions[i][0], vertex_positions[i][1],
                     vertex_positions[i][2]);
        b.write_pod<int8_t>(vertex_bones[i]);
        b.write_pod<uint8_t>(1); // ref count
    }

    /* 1 triangle */
    b.write_pod<uint16_t>(1);
    b.write_pod<uint16_t>(0); // flags
    b.write_pod<uint16_t>(0);
    b.write_pod<uint16_t>(1);
    b.write_pod<uint16_t>(2);
    for(int i = 0; i < 3; ++i) {
        b.write_vec3(0.0f, 0.0f, 1.0f); // normals
    }
    b.write_pod<float>(0.0f); // s
    b.write_pod<float>(1.0f);
    b.write_pod<float>(0.0f);
    b.write_pod<float>(0.0f); // t
    b.write_pod<float>(0.0f);
    b.write_pod<float>(1.0f);
    b.write_pod<uint8_t>(0); // smoothing group
    b.write_pod<uint8_t>(0); // group index

    /* 1 group, referencing the only triangle and the only material */
    b.write_pod<uint16_t>(1);
    b.write_pod<uint8_t>(0); // flags
    b.write_name("Group", 32);
    b.write_pod<uint16_t>(1);
    b.write_pod<uint16_t>(0); // triangle index
    b.write_pod<char>(0);     // material index

    /* 1 material, with no texture so that nothing is loaded from disk */
    b.write_pod<uint16_t>(1);
    b.write_name("Mat", 32);
    for(int i = 0; i < 4; ++i) {
        // ambient, diffuse, specular, emissive
        b.write_pod<float>(1.0f);
        b.write_pod<float>(1.0f);
        b.write_pod<float>(1.0f);
        b.write_pod<float>(1.0f);
    }
    b.write_pod<float>(32.0f); // shininess
    b.write_pod<float>(0.0f);  // transparency
    b.write_pod<uint8_t>(0);   // mode
    b.write_name("", 128);     // texture
    b.write_name("", 128);     // alphamap

    /* Animation header: 20 frames at 10fps, so 2 seconds long */
    b.write_pod<float>(10.0f); // fps
    b.write_pod<float>(0.0f);  // current time
    b.write_pod<int32_t>(20);  // total frames

    /* 2 joints. "root" is animated, "child" just sits at its rest pose */
    b.write_pod<uint16_t>(2);

    b.write_pod<uint8_t>(0); // flags
    b.write_name("root", 32);
    b.write_name("", 32);           // no parent
    b.write_vec3(0.0f, 0.0f, 0.0f); // rest rotation
    b.write_vec3(0.0f, 0.0f, 0.0f); // rest position
    b.write_pod<uint16_t>(2);       // rotation keyframes
    b.write_pod<uint16_t>(2);       // position keyframes
    b.write_pod<float>(0.0f);
    b.write_vec3(0.0f, 0.0f, 0.0f);
    b.write_pod<float>(1.0f);
    b.write_vec3(0.0f, 0.0f, 0.0f);
    b.write_pod<float>(0.0f);
    b.write_vec3(0.0f, 0.0f, 0.0f);
    b.write_pod<float>(1.0f);
    b.write_vec3(0.0f, 2.0f, 0.0f);

    b.write_pod<uint8_t>(0); // flags
    b.write_name("child", 32);
    b.write_name("root", 32);
    b.write_vec3(0.0f, 0.0f, 0.0f); // rest rotation
    b.write_vec3(0.0f, 1.0f, 0.0f); // rest position
    b.write_pod<uint16_t>(0);
    b.write_pod<uint16_t>(0);

    return b.bytes();
}

static Path write_test_ms3d() {
    auto path = Path::system_temp_dir().append("simulant_test.ms3d");

    auto data = generate_ms3d();
    std::ofstream out(path.str().c_str(), std::ios::binary);
    out.write(reinterpret_cast<const char*>(&data[0]), data.size());
    out.close();

    return path;
}

class MS3DLoaderTests: public test::SimulantTestCase {
public:
    void test_basic_loading() {
        auto prefab = scene->assets->load_prefab(write_test_ms3d());

        assert_is_not_null(prefab.get());

        /* One Armature for the mesh, plus a node per joint */
        assert_equal(prefab->node_count(), 3u);
        assert_true(prefab->has_animations());
        assert_equal(prefab->animation_count(), 1u);
    }

    void test_mesh_is_skinned() {
        auto prefab = scene->assets->load_prefab(write_test_ms3d());
        assert_is_not_null(prefab.get());

        auto mesh_node = prefab->node(0);
        assert_true(bool(mesh_node));
        assert_equal(mesh_node.value().node_type_name.str(),
                     std::string("armature"));

        auto mesh = mesh_node.value()
                        .params.get<MeshRef>("mesh")
                        .value_or(MeshPtr())
                        .lock();
        assert_is_not_null(mesh.get());

        /* MS3D shares vertices between faces, so the loader duplicates them
         * per triangle corner */
        assert_equal(mesh->vertex_data->count(), 3u);
        assert_equal(mesh->first_submesh()->index_data->count(), 3u);

        assert_true(mesh->is_skinned());
        assert_is_not_null(mesh->skin.get());

        assert_equal(mesh->skin->inverse_bind_matrices.size(), 2u);

        /* Joint N in the file maps to prefab node (N + 1), and carries N as
         * its index into the inverse bind matrices */
        for(uint32_t i = 0; i < 2; ++i) {
            auto joint_node = prefab->node(i + 1);
            assert_true(bool(joint_node));
            assert_equal(joint_node.value().node_type_name.str(),
                         std::string("joint"));
            assert_equal(
                joint_node.value().params.get<int>("joint_index").value_or(-1),
                (int)i);
        }
    }

    void test_joint_hierarchy_and_animation() {
        auto prefab = scene->assets->load_prefab(write_test_ms3d());
        assert_is_not_null(prefab.get());

        auto instance = scene->create_child<PrefabInstance>(prefab);
        assert_is_not_null(instance);

        auto root = instance->find_descendent_with_name("root");
        assert_is_not_null(root);

        auto child = instance->find_descendent_with_name("child");
        assert_is_not_null(child);

        /* The child joint should be parented to the root joint, at its
         * rest position relative to it */
        assert_equal(child->parent(), root);
        assert_close(child->transform->translation().y, 1.0f, 0.0001f);

        auto controller = instance->find_mixin<AnimationController>();
        assert_is_not_null(controller);

        auto names = controller->animation_names();
        assert_equal(names.size(), 1u);
        assert_equal(names[0], std::string("default"));

        /* The root joint translates from (0, 0, 0) to (0, 2, 0) over the
         * first second of the animation */
        assert_true(controller->play("default"));
        controller->seek(0.5f);
        assert_close(root->transform->translation().y, 1.0f, 0.0001f);

        controller->seek(1.0f);
        assert_close(root->transform->translation().y, 2.0f, 0.0001f);
    }
};

} // namespace
