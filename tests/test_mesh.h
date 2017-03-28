#ifndef TEST_MESH_H
#define TEST_MESH_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"
#include "global.h"

class MeshTest : public SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        camera_id_ = window->new_camera();
        stage_id_ = window->new_stage();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        window->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
    }

    smlt::MeshID generate_test_mesh(smlt::StagePtr stage) {
        smlt::MeshID mid = stage->assets->new_mesh(smlt::VertexSpecification::POSITION_ONLY);
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
        smlt::SubMesh* submesh = first_mesh_;

        submesh->index_data->index(0);
        submesh->index_data->index(1);
        submesh->index_data->index(2);

        submesh->index_data->index(0);
        submesh->index_data->index(2);
        submesh->index_data->index(3);
        submesh->index_data->done();

        //Draw a line between the first two vertices
        smlt::SubMesh* sm = mesh->new_submesh("test2", smlt::MESH_ARRANGEMENT_LINES);
        sm->index_data->index(0);
        sm->index_data->index(1);
        sm->index_data->done();

        Vec3 expected_min(-1.0, -1.0, 0.0);
        Vec3 expected_max( 1.0, -1.0, 0.0);

        auto box = sm->aabb();
        assert_true(box.min == expected_min);
        assert_true(box.max == expected_max);

        return mid;
    }

    void test_create_mesh_from_submesh() {
        auto stage = window->stage(stage_id_);
        auto mesh = stage->assets->mesh(generate_test_mesh(stage));
        auto submesh = mesh->first_submesh();

        auto second_mesh_id = stage->assets->new_mesh_from_submesh(submesh);
        auto second_mesh = stage->assets->mesh(second_mesh_id);

        assert_equal(second_mesh->first_submesh()->index_data->count(), submesh->index_data->count());
        assert_equal(second_mesh->first_submesh()->arrangement(), submesh->arrangement());
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

        smlt::ActorID mid = stage->new_actor();
        auto actor = stage->actor(mid);

        this->assert_true(actor->id() != 0); //Make sure we set an id for the mesh
        this->assert_true(actor->auto_id() != 0); //Make sure we set a unique ID for the object
        this->assert_true(!actor->data->exists("data"));
        actor->data->stash((int)0xDEADBEEF, "data");
        this->assert_true(actor->data->exists("data"));
        this->assert_equal((int)0xDEADBEEF, actor->data->get<int>("data"));

        stage->delete_actor(mid);

        this->assert_true(!stage->has_actor(mid));
    }

    void test_deleting_entities_deletes_children() {
        auto stage = window->stage(stage_id_);

        smlt::ActorID mid = stage->new_actor(); //Create the root mesh
        smlt::ActorID cid1 = stage->new_actor_with_parent(mid); //Create a child
        smlt::ActorID cid2 = stage->new_actor_with_parent(cid1); //Create a child of the child

        this->assert_equal((uint32_t)1, stage->actor(mid)->count_children());
        this->assert_equal((uint32_t)1, stage->actor(cid1)->count_children());
        this->assert_equal((uint32_t)0, stage->actor(cid2)->count_children());

        stage->delete_actor(mid);
        this->assert_true(!stage->has_actor(mid));
        this->assert_true(!stage->has_actor(cid1));
        this->assert_true(!stage->has_actor(cid2));
    }

    void test_basic_usage() {
        auto stage = window->stage(stage_id_);
        auto mesh = stage->assets->mesh(generate_test_mesh(stage));

        auto& data = mesh->shared_data;

        assert_true(data->specification().has_positions());
        assert_true(!data->specification().has_normals());
        assert_true(!data->specification().has_texcoord0());
        assert_true(!data->specification().has_texcoord1());
        assert_true(!data->specification().has_texcoord2());
        assert_true(!data->specification().has_texcoord3());
        assert_true(!data->specification().has_diffuse());
        assert_true(!data->specification().has_specular());
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

        smlt::SubMesh* sm = actor->subactor(0).submesh();

        //Likewise for subentities, they should just proxy to the submesh
        assert_equal(sm->material_id(), actor->subactor(0).material_id());
        assert_equal(sm->index_data.get(), actor->subactor(0).index_data.get());
        assert_equal(sm->vertex_data.get(), actor->subactor(0).vertex_data.get());

        //We should be able to override the material on a subactor though
        actor->subactor(0).override_material_id(smlt::MaterialID(1));

        assert_equal(smlt::MaterialID(1), actor->subactor(0).material_id());
    }

    void test_scene_methods() {
        auto stage = window->stage(stage_id_);

        smlt::MeshID mesh_id = stage->assets->new_mesh(smlt::VertexSpecification::POSITION_ONLY); //Create a mesh
        auto actor = stage->actor(stage->new_actor_with_mesh(mesh_id));

        assert_true(mesh_id == actor->mesh()->id());
    }

    void test_animated_mesh_initialization() {
        auto stage = window->stage(stage_id_);

        const int num_frames = 3;

        auto mesh_id = stage->assets->new_animated_mesh(
            smlt::VertexSpecification::POSITION_ONLY,
            smlt::MESH_ANIMATION_TYPE_VERTEX_MORPH,
            num_frames
        );

        auto mesh = stage->assets->mesh(mesh_id);

        assert_equal(mesh->animation_frames(), 3);
        assert_equal(mesh->animation_type(), smlt::MESH_ANIMATION_TYPE_VERTEX_MORPH);
        assert_true(mesh->is_animated());
    }

    void test_cubic_texture_generation() {
        auto stage = window->stage(stage_id_);

        auto mesh_id = stage->assets->new_mesh_as_box(10.0f, 10.0f, 10.0f);
        stage->assets->mesh(mesh_id)->first_submesh()->generate_texture_coordinates_cube();

        auto& vd = *stage->assets->mesh(mesh_id)->first_submesh()->vertex_data.get();

        // Neg Z
        assert_equal(smlt::Vec2((1.0 / 3.0), 0), vd.texcoord0_at<smlt::Vec2>(0));
        assert_equal(smlt::Vec2((2.0 / 3.0), 0), vd.texcoord0_at<smlt::Vec2>(1));
        assert_equal(smlt::Vec2((2.0 / 3.0), (1.0 / 4.0)), vd.texcoord0_at<smlt::Vec2>(2));
        assert_equal(smlt::Vec2((1.0 / 3.0), (1.0 / 4.0)), vd.texcoord0_at<smlt::Vec2>(3));

        // Pos Z
        assert_equal(smlt::Vec2((1.0 / 3.0), (2.0 / 4.0)), vd.texcoord0_at<smlt::Vec2>(4));
        assert_equal(smlt::Vec2((2.0 / 3.0), (2.0 / 4.0)), vd.texcoord0_at<smlt::Vec2>(5));
        assert_equal(smlt::Vec2((2.0 / 3.0), (3.0 / 4.0)), vd.texcoord0_at<smlt::Vec2>(6));
        assert_equal(smlt::Vec2((1.0 / 3.0), (3.0 / 4.0)), vd.texcoord0_at<smlt::Vec2>(7));

        // Neg X
        assert_equal(smlt::Vec2(0, 2.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(8));
        assert_equal(smlt::Vec2(1.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(9));
        assert_equal(smlt::Vec2(1.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(10));
        assert_equal(smlt::Vec2(0, 3.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(11));

        // Pos X
        assert_equal(smlt::Vec2(2.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(12));
        assert_equal(smlt::Vec2(3.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(13));
        assert_equal(smlt::Vec2(3.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(14));
        assert_equal(smlt::Vec2(2.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(15));

        // Neg Y
        assert_equal(smlt::Vec2(1.0 / 3.0, 1.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(16));
        assert_equal(smlt::Vec2(2.0 / 3.0, 1.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(17));
        assert_equal(smlt::Vec2(2.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(18));
        assert_equal(smlt::Vec2(1.0 / 3.0, 2.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(19));

        // Pos Y
        assert_equal(smlt::Vec2(1.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(20));
        assert_equal(smlt::Vec2(2.0 / 3.0, 3.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(21));
        assert_equal(smlt::Vec2(2.0 / 3.0, 4.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(22));
        assert_equal(smlt::Vec2(1.0 / 3.0, 4.0 / 4.0), vd.texcoord0_at<smlt::Vec2>(23));
    }

private:
    smlt::CameraID camera_id_;
    smlt::StageID stage_id_;

    smlt::SubMesh* first_mesh_;
};

#endif // TEST_MESH_H
