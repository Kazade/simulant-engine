#ifndef TEST_MESH_H
#define TEST_MESH_H

#include "kglt/kazbase/testing.h"

#include "kglt/kglt.h"
#include "global.h"

class MeshTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    MeshID generate_test_mesh(kglt::Stage& scene) {
        kglt::MeshID mid = scene.new_mesh();
        kglt::MeshPtr mesh = scene.mesh(mid).lock();

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

        kglt::SubMesh& submesh = mesh->submesh(mesh->new_submesh(MaterialID()));

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

        assert_true(kmVec3AreEqual(&sm.bounds().min, &expected_min));
        assert_true(kmVec3AreEqual(&sm.bounds().max, &expected_max));

        return mid;
    }

    void test_user_data_works() {
        kglt::Stage& scene = window->scene().stage();

        kglt::ActorID mid = scene.new_actor();
        kglt::Actor& actor = scene.actor(mid);

        this->assert_true(actor.id() != 0); //Make sure we set an id for the mesh
        this->assert_true(actor.uuid() != 0); //Make sure we set a unique ID for the object
        this->assert_true(!actor.exists("data"));
        actor.stash((int)0xDEADBEEF, "data");
        this->assert_true(actor.exists("data"));
        this->assert_equal((int)0xDEADBEEF, actor.get<int>("data"));

        scene.delete_actor(mid);

        this->assert_true(!scene.has_actor(mid));
    }

    void test_deleting_entities_deletes_children() {
        kglt::Stage& scene = window->scene().stage();

        kglt::ActorID mid = scene.new_actor(); //Create the root mesh        
        kglt::ActorID cid1 = scene.new_actor_with_parent(scene.actor(mid)); //Create a child
        kglt::ActorID cid2 = scene.new_actor_with_parent(scene.actor(cid1)); //Create a child of the child

        this->assert_equal((uint32_t)1, scene.actor(mid).child_count());
        this->assert_equal((uint32_t)1, scene.actor(cid1).child_count());
        this->assert_equal((uint32_t)0, scene.actor(cid2).child_count());

        scene.delete_actor(mid);
        this->assert_true(!scene.has_actor(mid));
        this->assert_true(!scene.has_actor(cid1));
        this->assert_true(!scene.has_actor(cid2));
    }

    void test_procedural_rectangle_outline() {
        kglt::Stage& scene = window->scene().stage();

        kglt::MeshID mid = scene.new_mesh();
        kglt::MeshPtr mesh = scene.mesh(mid).lock();

        this->assert_equal(0, mesh->shared_data().count());
        kglt::SubMeshIndex idx = kglt::procedural::mesh::rectangle_outline(mesh, 1.0, 1.0);

        this->assert_equal(kglt::MESH_ARRANGEMENT_LINE_STRIP, mesh->submesh(idx).arrangement());
        this->assert_equal(4, mesh->shared_data().count());
        this->assert_equal(5, mesh->submesh(idx).index_data().count());
    }

    void test_basic_usage() {
        kglt::Stage& scene = window->scene().stage();
        kglt::MeshPtr mesh = scene.mesh(generate_test_mesh(scene)).lock();

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
        kglt::Stage& scene = window->scene().stage();

        kglt::MeshPtr mesh = scene.mesh(generate_test_mesh(scene)).lock();

        kglt::Actor& actor = scene.actor(scene.new_actor());

        assert_true(!actor.has_mesh());

        actor.set_mesh(mesh->id());

        assert_true(actor.has_mesh());

        //The actor's MeshID should match the mesh we set
        assert_true(mesh->id() == actor.mesh().lock()->id());

        //The actor should report the same data as the mesh, the same subactor count
        //as well as the same shared vertex data
        assert_equal(mesh->submesh_ids().size(), actor.subactor_count());
        assert_true(mesh->shared_data().count() == actor.shared_data().count());

        kglt::SubMeshIndex idx = mesh->submesh_ids()[0];

        //Likewise for subentities, they should just proxy to the submesh
        assert_equal(mesh->submesh(idx).material_id(), actor.subactor(0).material_id());
        assert_true(mesh->submesh(idx).index_data() == actor.subactor(0).index_data());
        assert_true(mesh->submesh(idx).vertex_data() == actor.subactor(0).vertex_data());

        //We should be able to override the material on a subactor though
        actor.subactor(0).override_material_id(kglt::MaterialID(1));

        assert_equal(MaterialID(1), actor.subactor(0).material_id());
    }

    void test_scene_methods() {
        kglt::Stage& scene = window->scene().stage();

        kglt::MeshPtr mesh = scene.mesh(scene.new_mesh()).lock(); //Create a mesh
        kglt::Actor& actor = scene.actor(scene.new_actor(mesh->id()));

        assert_true(mesh->id() == actor.mesh().lock()->id());
    }
};

#endif // TEST_MESH_H
