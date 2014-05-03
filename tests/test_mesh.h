#ifndef TEST_MESH_H
#define TEST_MESH_H

#include <kazbase/logging.h>
#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"

class MeshTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    kglt::MeshID generate_test_mesh(AutoWeakPtr<kglt::Stage> stage) {
        kglt::MeshID mid = stage->new_mesh();
        auto mesh = stage->mesh(mid);

        kglt::VertexData& data = mesh->shared_data();

        data.position(-1.0, -1.0, 0.0);
        data.move_next();

        data.position( 1.0, -1.0, 0.0);
        data.move_next();

        data.position( 1.0, 1.0, 0.0);
        data.move_next();

        data.position(-1.0, 1.0, 0.0);
        data.move_next();

        data.done();

        kglt::SubMesh& submesh = mesh->submesh(mesh->new_submesh(kglt::MaterialID()));

        submesh.index_data().index(0);
        submesh.index_data().index(1);
        submesh.index_data().index(2);

        submesh.index_data().index(0);
        submesh.index_data().index(2);
        submesh.index_data().index(3);
        submesh.index_data().done();

        //Draw a line between the first two vertices
        kglt::SubMesh& sm = mesh->submesh(mesh->new_submesh(kglt::MaterialID(), kglt::MESH_ARRANGEMENT_LINES));
        sm.index_data().index(0);
        sm.index_data().index(1);
        sm.index_data().done();

        kmVec3 expected_min, expected_max;
        kmVec3Fill(&expected_min, -1.0, -1.0, 0.0);
        kmVec3Fill(&expected_max, 1.0, -1.0, 0.0);

        auto box = sm.aabb();
        assert_true(kmVec3AreEqual(&box.min, &expected_min));
        assert_true(kmVec3AreEqual(&box.max, &expected_max));

        return mid;
    }

    void test_user_data_works() {
        auto stage = window->stage();

        kglt::ActorID mid = stage->new_actor();
        auto actor = stage->actor(mid);

        this->assert_true(actor->id() != 0); //Make sure we set an id for the mesh
        this->assert_true(actor->uuid() != 0); //Make sure we set a unique ID for the object
        this->assert_true(!actor->exists("data"));
        actor->stash((int)0xDEADBEEF, "data");
        this->assert_true(actor->exists("data"));
        this->assert_equal((int)0xDEADBEEF, actor->get<int>("data"));

        stage->delete_actor(mid);

        this->assert_true(!stage->has_actor(mid));
    }

    void test_deleting_entities_deletes_children() {
        auto stage = window->stage();

        kglt::ActorID mid = stage->new_actor(); //Create the root mesh
        kglt::ActorID cid1 = stage->new_actor_with_parent(mid); //Create a child
        kglt::ActorID cid2 = stage->new_actor_with_parent(cid1); //Create a child of the child

        this->assert_equal((uint32_t)1, stage->actor(mid)->children().size());
        this->assert_equal((uint32_t)1, stage->actor(cid1)->children().size());
        this->assert_equal((uint32_t)0, stage->actor(cid2)->children().size());

        stage->delete_actor(mid);
        this->assert_true(!stage->has_actor(mid));
        this->assert_true(!stage->has_actor(cid1));
        this->assert_true(!stage->has_actor(cid2));
    }

    void test_procedural_rectangle_outline() {
        auto stage = window->stage();

        kglt::MeshID mid = stage->new_mesh();
        auto mesh = stage->mesh(mid);

        this->assert_equal(0, mesh->shared_data().count());
        kglt::SubMeshIndex idx = kglt::procedural::mesh::rectangle_outline(mesh, 1.0, 1.0);

        this->assert_equal(kglt::MESH_ARRANGEMENT_LINE_STRIP, mesh->submesh(idx).arrangement());
        this->assert_equal(4, mesh->shared_data().count());
        this->assert_equal(5, mesh->submesh(idx).index_data().count());
    }

    void test_basic_usage() {
        auto stage = window->stage();
        auto mesh = stage->mesh(generate_test_mesh(stage));

        kglt::VertexData& data = mesh->shared_data();

        assert_true(data.has_positions());
        assert_true(!data.has_normals());
        assert_true(!data.has_texcoord0());
        assert_true(!data.has_texcoord1());
        assert_true(!data.has_texcoord2());
        assert_true(!data.has_texcoord3());
        assert_true(!data.has_texcoord4());
        assert_true(!data.has_diffuse());
        assert_true(!data.has_specular());
        assert_equal(4, data.count());

        assert_equal((uint32_t)2, mesh->submesh_ids().size());
    }

    void test_actor_from_mesh() {
        auto stage = window->stage();

        auto mesh = stage->mesh(generate_test_mesh(stage));

        auto actor = stage->actor(stage->new_actor());

        assert_true(!actor->has_mesh());

        actor->set_mesh(mesh->id());

        assert_true(actor->has_mesh());

        //The actor's MeshID should match the mesh we set
        assert_true(mesh->id() == actor->mesh()->id());

        //The actor should report the same data as the mesh, the same subactor count
        //as well as the same shared vertex data
        assert_equal(mesh->submesh_ids().size(), actor->subactor_count());
        assert_true(mesh->shared_data().count() == actor->shared_data().count());

        kglt::SubMeshIndex idx = mesh->submesh_ids()[0];

        //Likewise for subentities, they should just proxy to the submesh
        assert_equal(mesh->submesh(idx).material_id(), actor->subactor(0).material_id());
        assert_true(mesh->submesh(idx).index_data() == actor->subactor(0).index_data());
        assert_true(mesh->submesh(idx).vertex_data() == actor->subactor(0).vertex_data());

        //We should be able to override the material on a subactor though
        actor->subactor(0).override_material_id(kglt::MaterialID(1));

        assert_equal(kglt::MaterialID(1), actor->subactor(0).material_id());
    }

    void test_scene_methods() {
        auto stage = window->stage();

        kglt::MeshID mesh_id = stage->new_mesh(); //Create a mesh
        auto actor = stage->actor(stage->new_actor(mesh_id));

        assert_true(mesh_id == actor->mesh()->id());
    }
};

#endif // TEST_MESH_H
