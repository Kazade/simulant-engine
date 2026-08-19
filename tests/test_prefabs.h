#pragma once

#include "simulant/simulant.h"

namespace {

using namespace smlt;

class PrefabTests: public test::SimulantTestCase {
public:
    void test_prefab_from_stage_nodes() {
        auto mesh =
            scene->assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->create_submesh_as_cube("cube", scene->assets->create_material(),
                                     50.0f);

        auto a0 = scene->create_child<smlt::Actor>(mesh);
        a0->create_child<smlt::Actor>(mesh);

        // Create a prefab the nodes below and including a0
        auto prefab = scene->assets->create_prefab(a0);

        assert_equal(prefab->node_count(), 2u);

        // Create a PrefabInstance from the prefab, which will instantiate the
        // nodes beneath
        auto instance = scene->create_child<PrefabInstance>(prefab);

        auto c0 = dynamic_cast<const Actor* const>(instance->child_at(0));
        assert_equal(c0->base_mesh(), mesh);

        auto c1 = dynamic_cast<const Actor* const>(c0->child_at(0));
        assert_equal(c1->base_mesh(), mesh);
    }

    static ArmaturePtr first_armature(StageNode* root) {
        auto found = root->find_descendents_by_types({Armature::Meta::node_type});
        return found.empty() ? nullptr : static_cast<ArmaturePtr>(found[0]);
    }

    /* A skinned glTF should come out as an Armature holding the mesh, with
     * the skin's joints below it as Joint nodes. */
    void test_skinned_prefab_builds_an_armature() {
        auto prefab = scene->assets->load_prefab(
            "assets/samples/khronos/RiggedSimple.glb");

        auto instance = scene->create_child<PrefabInstance>(prefab);

        auto armature = first_armature(instance);
        assert_is_not_null(armature);

        assert_equal(armature->mesh_count(), 1u);
        assert_true(armature->source_mesh()->is_skinned());

        /* RiggedSimple has a two bone skeleton, both below the armature */
        assert_equal(armature->joint_count(), 2u);
        assert_is_not_null(armature->joint(0));
        assert_is_not_null(armature->joint(1));
        assert_equal(armature->joint(0)->armature(), armature);
        assert_equal(armature->joint(1)->parent(), (StageNode*)armature->joint(0));

        /* Meshes only ever hold the bind pose. Posing no longer produces a
         * persistent output mesh - it's deferred to the renderer - but a
         * synchronous CPU readback of the current pose is still available
         * for tools/tests/gameplay code that need it (e.g. picking). */
        armature->update_skinning();

        VertexData posed(
            armature->source_mesh()->vertex_data->vertex_specification());
        assert_true(armature->read_back_pose(0, posed));
        assert_equal(posed.count(), armature->source_mesh()->vertex_data->count());
    }

    /* Instantiating the same skinned prefab more than once used to corrupt
     * every earlier instance: Mesh objects loaded from a glTF are shared by
     * pointer, and skinning wrote straight back into that shared Mesh, so
     * every instance fought over one mutable vertex buffer. Each Armature
     * now keeps its own joint matrices and never writes to the (shared,
     * immutable) source mesh at all. */
    void test_multiple_instances_of_skinned_prefab_dont_share_mesh_state() {
        auto prefab = scene->assets->load_prefab(
            "assets/samples/khronos/RiggedSimple.glb");
        assert_true(prefab->has_animations());

        auto instance0 = scene->create_child<PrefabInstance>(prefab);
        auto instance1 = scene->create_child<PrefabInstance>(prefab);

        auto armature0 = first_armature(instance0);
        auto armature1 = first_armature(instance1);

        assert_is_not_null(armature0);
        assert_is_not_null(armature1);

        // The bind pose is immutable, so it stays shared between the two
        // instances - that's the whole memory-saving point.
        assert_equal(armature0->source_mesh(), armature1->source_mesh());
        assert_equal(armature0->source_mesh()->skin,
                     armature1->source_mesh()->skin);

        // Each armature drives its own skeleton
        assert_not_equal(armature0->joint(0), armature1->joint(0));
        assert_equal(armature0->joint(0)->armature(), armature0);
        assert_equal(armature1->joint(0)->armature(), armature1);

        auto bind_pose_vertex =
            *armature0->source_mesh()->vertex_data->position_at<Vec3>(0);

        armature0->joint(0)->transform->set_translation(Vec3(0, 50, 0));
        armature0->update_skinning();
        armature1->update_skinning();

        // Posing must never write back into the shared source mesh.
        assert_equal(*armature0->source_mesh()->vertex_data->position_at<Vec3>(0),
                     bind_pose_vertex);

        VertexData pose0(
            armature0->source_mesh()->vertex_data->vertex_specification());
        VertexData pose1(
            armature1->source_mesh()->vertex_data->vertex_specification());
        assert_true(armature0->read_back_pose(0, pose0));
        assert_true(armature1->read_back_pose(0, pose1));

        // The two instances must have posed independently.
        assert_not_equal(*pose0.position_at<Vec3>(0), *pose1.position_at<Vec3>(0));
    }

    /* Posing is done in armature space, so two instances at different
     * places in the scene produce the same local vertex positions - and
     * moving a joint on one must not disturb the other. */
    void test_posing_one_instance_doesnt_affect_another() {
        auto prefab = scene->assets->load_prefab(
            "assets/samples/khronos/RiggedSimple.glb");

        auto instance0 = scene->create_child<PrefabInstance>(prefab);
        auto instance1 = scene->create_child<PrefabInstance>(prefab);
        instance1->transform->set_translation(Vec3(100, 0, 0));

        auto armature0 = first_armature(instance0);
        auto armature1 = first_armature(instance1);

        armature0->update_skinning();
        armature1->update_skinning();

        auto vertex_of = [](ArmaturePtr a) {
            VertexData posed(
                a->source_mesh()->vertex_data->vertex_specification());
            a->read_back_pose(0, posed);
            return *posed.position_at<Vec3>(0);
        };

        auto before = vertex_of(armature1);
        assert_close(vertex_of(armature0).x, before.x, 0.0001f);
        assert_close(vertex_of(armature0).y, before.y, 0.0001f);
        assert_close(vertex_of(armature0).z, before.z, 0.0001f);

        armature0->joint(0)->transform->set_translation(Vec3(0, 50, 0));
        armature0->update_skinning();

        auto after = vertex_of(armature1);
        assert_close(after.x, before.x, 0.0001f);
        assert_close(after.y, before.y, 0.0001f);
        assert_close(after.z, before.z, 0.0001f);
    }
};

} // namespace
