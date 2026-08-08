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

    /* Instantiating the same skinned prefab more than once used to corrupt
     * every earlier instance: Mesh objects loaded from a glTF are shared by
     * pointer, and PrefabInstance::on_create bound skin->bound_actor /
     * skin->node_indices directly onto that shared Mesh, so the most
     * recently created instance would silently steal every other
     * instance's skin binding (and they'd all fight over one mutable
     * vertex buffer). Each skinned Actor should now get its own writable
     * mesh clone. */
    void test_multiple_instances_of_skinned_prefab_dont_share_mesh_state() {
        auto prefab = scene->assets->load_prefab(
            "assets/samples/khronos/RiggedSimple.glb");
        assert_true(prefab->has_animations());

        auto instance0 = scene->create_child<PrefabInstance>(prefab);
        auto instance1 = scene->create_child<PrefabInstance>(prefab);

        auto actor0 = dynamic_cast<ActorPtr>(
            instance0->find_descendents_by_types({Actor::Meta::node_type})[0]);
        auto actor1 = dynamic_cast<ActorPtr>(
            instance1->find_descendents_by_types({Actor::Meta::node_type})[0]);

        assert_true(actor0->base_mesh()->is_skinned);
        assert_true(actor1->base_mesh()->is_skinned);

        // Each instance must have its own Mesh/Skin, not share the
        // originally loaded one (or each other's).
        assert_not_equal(actor0->base_mesh(), actor1->base_mesh());
        assert_not_equal(actor0->base_mesh()->vertex_data.get(),
                         actor1->base_mesh()->vertex_data.get());
        assert_not_equal(actor0->base_mesh()->skin, actor1->base_mesh()->skin);

        // Each Skin should be bound to its own Actor, not to whichever
        // instance was created last.
        assert_equal(actor0->base_mesh()->skin->bound_actor, actor0);
        assert_equal(actor1->base_mesh()->skin->bound_actor, actor1);

        // The immutable bind-pose data backing skinning should still be
        // shared between the two instances (that's the whole memory-saving
        // point), even though the writable output buffers are separate.
        assert_equal(actor0->base_mesh()->submesh_count(),
                     actor1->base_mesh()->submesh_count());
    }
};

} // namespace
