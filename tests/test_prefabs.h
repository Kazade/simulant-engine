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

        /* Meshes only ever hold the bind pose - posing happens into the
         * armature's own output mesh */
        assert_not_equal(armature->skinned_mesh(), armature->source_mesh());
        assert_equal(armature->skinned_mesh()->submesh_count(),
                     armature->source_mesh()->submesh_count());
    }

    /* Instantiating the same skinned prefab more than once used to corrupt
     * every earlier instance: Mesh objects loaded from a glTF are shared by
     * pointer, and skinning wrote straight back into that shared Mesh, so
     * every instance fought over one mutable vertex buffer. Each Armature
     * should now own the mesh it poses. */
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

        // The posed output must not be, or the two instances would
        // overwrite each other every frame.
        assert_not_equal(armature0->skinned_mesh(), armature1->skinned_mesh());
        assert_not_equal(armature0->skinned_mesh()->vertex_data.get(),
                         armature1->skinned_mesh()->vertex_data.get());

        // Each armature drives its own skeleton
        assert_not_equal(armature0->joint(0), armature1->joint(0));
        assert_equal(armature0->joint(0)->armature(), armature0);
        assert_equal(armature1->joint(0)->armature(), armature1);
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
            return *a->skinned_mesh()->vertex_data->position_at<Vec3>(0);
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
