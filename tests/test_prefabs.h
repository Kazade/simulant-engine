#pragma once

#include "simulant/simulant.h"

namespace {

using namespace smlt;

class PrefabTests: public test::SimulantTestCase {
public:
    void test_prefab_from_stage_nodes() {
        auto mesh = scene->assets->create_mesh(smlt::VertexFormat::standard());
        mesh->create_submesh_as_cube("cube", scene->assets->create_material(),
                                     50.0f);

        auto a0 = scene->create_child<smlt::Actor>(mesh);
        auto a1 = a0->create_child<smlt::Actor>(mesh);

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
};

} // namespace
