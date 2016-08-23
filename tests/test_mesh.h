#ifndef TEST_MESH_H
#define TEST_MESH_H

#include <kazbase/logging.h>
#include "kaztest/kaztest.h"

#include "kglt/kglt.h"
#include "global.h"

class MeshTest : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        camera_id_ = window->new_camera();
        stage_id_ = window->new_stage();
    }

    void tear_down() {
        KGLTTestCase::tear_down();
        window->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
    }

    kglt::MeshID generate_test_mesh(kglt::StagePtr stage) {
        kglt::MeshID mid = stage->assets->new_mesh(kglt::VertexSpecification::POSITION_ONLY);
        auto mesh = stage->assets->mesh(mid);

        auto& data = mesh->shared_data;

        data->position(-1.0, -1.0, 0.0);
        data->move_next();

        data->position( 1.0, -1.0, 0.0);
        data->move_next();

        data->position( 1.0, 1.0, 0.0);
        data->move_next();

        data->position(-1.0, 1.0, 0.0);
        data->move_next();

        data->done();

        first_mesh_ = mesh->new_submesh("test");
        kglt::SubMesh* submesh = first_mesh_;

        submesh->index_data->index(0);
        submesh->index_data->index(1);
        submesh->index_data->index(2);

        submesh->index_data->index(0);
        submesh->index_data->index(2);
        submesh->index_data->index(3);
        submesh->index_data->done();

        //Draw a line between the first two vertices
        kglt::SubMesh* sm = mesh->new_submesh("test2", kglt::MESH_ARRANGEMENT_LINES);
        sm->index_data->index(0);
        sm->index_data->index(1);
        sm->index_data->done();

        kmVec3 expected_min, expected_max;
        kmVec3Fill(&expected_min, -1.0, -1.0, 0.0);
        kmVec3Fill(&expected_max, 1.0, -1.0, 0.0);

        auto box = sm->aabb();
        assert_true(kmVec3AreEqual(&box.min, &expected_min));
        assert_true(kmVec3AreEqual(&box.max, &expected_max));

        return mid;
    }

    void test_mesh_normalization() {
        /*
         *  The normalize function scales the mesh so that it has a diameter of 1
         *  at its widest point. Useful for programmatically scaling stuff to the right
         *  size relative to other models
         */

        auto stage = window->stage(stage_id_);
        auto mesh = stage->assets->mesh(generate_test_mesh(stage));

        assert_close(2.0, mesh->diameter(), 0.00001);
        mesh->normalize();
        assert_close(1.0, mesh->diameter(), 0.00001);
    }

    void test_user_data_works() {
        auto stage = window->stage(stage_id_);

        kglt::ActorID mid = stage->new_actor();
        auto actor = stage->actor(mid);

        this->assert_true(actor->id() != 0); //Make sure we set an id for the mesh
        this->assert_true(actor->uuid() != 0); //Make sure we set a unique ID for the object
        this->assert_true(!actor->data->exists("data"));
        actor->data->stash((int)0xDEADBEEF, "data");
        this->assert_true(actor->data->exists("data"));
        this->assert_equal((int)0xDEADBEEF, actor->data->get<int>("data"));

        stage->delete_actor(mid);

        this->assert_true(!stage->has_actor(mid));
    }

    void test_deleting_entities_deletes_children() {
        auto stage = window->stage(stage_id_);

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

    void test_basic_usage() {
        auto stage = window->stage(stage_id_);
        auto mesh = stage->assets->mesh(generate_test_mesh(stage));

        auto& data = mesh->shared_data;

        assert_true(data->has_positions());
        assert_true(!data->has_normals());
        assert_true(!data->has_texcoord0());
        assert_true(!data->has_texcoord1());
        assert_true(!data->has_texcoord2());
        assert_true(!data->has_texcoord3());
        assert_true(!data->has_diffuse());
        assert_true(!data->has_specular());
        assert_equal(4, data->count());

        assert_equal((uint32_t)2, mesh->submesh_count());
    }

    void test_actor_from_mesh() {
        auto stage = window->stage(stage_id_);

        auto mesh = stage->assets->mesh(generate_test_mesh(stage));

        auto actor = stage->actor(stage->new_actor());

        assert_true(!actor->has_mesh());

        actor->set_mesh(mesh->id());

        assert_true(actor->has_mesh());

        //The actor's MeshID should match the mesh we set
        assert_true(mesh->id() == actor->mesh()->id());

        //The actor should report the same data as the mesh, the same subactor count
        //as well as the same shared vertex data
        assert_equal(mesh->submesh_count(), actor->subactor_count());
        assert_true(mesh->shared_data->count() == actor->shared_data->count());

        kglt::SubMesh* sm = actor->subactor(0).submesh();

        //Likewise for subentities, they should just proxy to the submesh
        assert_equal(sm->material_id(), actor->subactor(0).material_id());
        assert_equal(sm->index_data.get(), actor->subactor(0).index_data.get());
        assert_equal(sm->vertex_data.get(), actor->subactor(0).vertex_data.get());

        //We should be able to override the material on a subactor though
        actor->subactor(0).override_material_id(kglt::MaterialID(1));

        assert_equal(kglt::MaterialID(1), actor->subactor(0).material_id());
    }

    void test_scene_methods() {
        auto stage = window->stage(stage_id_);

        kglt::MeshID mesh_id = stage->assets->new_mesh(kglt::VertexSpecification::POSITION_ONLY); //Create a mesh
        auto actor = stage->actor(stage->new_actor_with_mesh(mesh_id));

        assert_true(mesh_id == actor->mesh()->id());
    }

    void test_cubic_texture_generation() {
        auto stage = window->stage(stage_id_);

        auto mesh_id = stage->assets->new_mesh_as_box(10.0f, 10.0f, 10.0f);
        stage->assets->mesh(mesh_id)->first_submesh()->generate_texture_coordinates_cube();

        auto& vd = *stage->assets->mesh(mesh_id)->first_submesh()->vertex_data.get();

        // Neg Z
        assert_equal(kglt::Vec2((1.0 / 3.0), 0), vd.texcoord0_at<kglt::Vec2>(0));
        assert_equal(kglt::Vec2((2.0 / 3.0), 0), vd.texcoord0_at<kglt::Vec2>(1));
        assert_equal(kglt::Vec2((2.0 / 3.0), (1.0 / 4.0)), vd.texcoord0_at<kglt::Vec2>(2));
        assert_equal(kglt::Vec2((1.0 / 3.0), (1.0 / 4.0)), vd.texcoord0_at<kglt::Vec2>(3));

        // Pos Z
        assert_equal(kglt::Vec2((1.0 / 3.0), (2.0 / 4.0)), vd.texcoord0_at<kglt::Vec2>(4));
        assert_equal(kglt::Vec2((2.0 / 3.0), (2.0 / 4.0)), vd.texcoord0_at<kglt::Vec2>(5));
        assert_equal(kglt::Vec2((2.0 / 3.0), (3.0 / 4.0)), vd.texcoord0_at<kglt::Vec2>(6));
        assert_equal(kglt::Vec2((1.0 / 3.0), (3.0 / 4.0)), vd.texcoord0_at<kglt::Vec2>(7));

        // Neg X
        assert_equal(kglt::Vec2(0, 2.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(8));
        assert_equal(kglt::Vec2(1.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(9));
        assert_equal(kglt::Vec2(1.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(10));
        assert_equal(kglt::Vec2(0, 3.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(11));

        // Pos X
        assert_equal(kglt::Vec2(2.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(12));
        assert_equal(kglt::Vec2(3.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(13));
        assert_equal(kglt::Vec2(3.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(14));
        assert_equal(kglt::Vec2(2.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(15));

        // Neg Y
        assert_equal(kglt::Vec2(1.0 / 3.0, 1.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(16));
        assert_equal(kglt::Vec2(2.0 / 3.0, 1.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(17));
        assert_equal(kglt::Vec2(2.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(18));
        assert_equal(kglt::Vec2(1.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(19));

        // Pos Y
        assert_equal(kglt::Vec2(1.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(20));
        assert_equal(kglt::Vec2(2.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(21));
        assert_equal(kglt::Vec2(2.0 / 3.0, 4.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(22));
        assert_equal(kglt::Vec2(1.0 / 3.0, 4.0 / 4.0), vd.texcoord0_at<kglt::Vec2>(23));
    }

private:
    kglt::CameraID camera_id_;
    kglt::StageID stage_id_;

    kglt::SubMesh* first_mesh_;
};

#endif // TEST_MESH_H
