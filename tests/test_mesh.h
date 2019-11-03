#ifndef TEST_MESH_H
#define TEST_MESH_H

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class MeshTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage_ = window->new_stage();
        camera_ = stage_->new_camera();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        stage_->destroy_camera(camera_->id());
        window->destroy_stage(stage_->id());
    }

    smlt::MeshID generate_test_mesh(smlt::StagePtr stage) {
        smlt::MeshID mid = stage_->assets->new_mesh(smlt::VertexSpecification::POSITION_ONLY, GARBAGE_COLLECT_NEVER);
        auto mesh = stage_->assets->mesh(mid);

        auto& data = mesh->vertex_data;

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
        assert_true(box.min() == expected_min);
        assert_true(box.max() == expected_max);

        mesh->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);

        return mid;
    }

    void test_create_mesh_from_submesh() {
        auto mesh = stage_->assets->mesh(generate_test_mesh(stage_));
        auto submesh = mesh->first_submesh();

        auto second_mesh_id = stage_->assets->new_mesh_from_submesh(submesh);
        auto second_mesh = stage_->assets->mesh(second_mesh_id);

        assert_equal(second_mesh->first_submesh()->index_data->count(), submesh->index_data->count());
        assert_equal(second_mesh->first_submesh()->arrangement(), submesh->arrangement());
    }

    void test_mesh_garbage_collection() {
        auto initial = stage_->assets->mesh_count();

        auto mesh1 = generate_test_mesh(stage_);
        auto mesh2 = generate_test_mesh(stage_);

        auto actor = stage_->new_actor_with_mesh(mesh1);
        actor->set_mesh(mesh2);

        stage_->assets->run_garbage_collection();

        assert_equal(stage_->assets->mesh_count(), initial + 1);

        stage_->destroy_actor(actor->id());

        window->run_frame();

        stage_->assets->run_garbage_collection();

        assert_equal(stage_->assets->mesh_count(), initial + 0);
    }

    void test_mesh_normalization() {
        /*
         *  The normalize function scales the mesh so that it has a diameter of 1
         *  at its widest point. Useful for programmatically scaling stuff to the right
         *  size relative to other models
         */

        auto mesh = stage_->assets->mesh(generate_test_mesh(stage_));

        assert_close(2.0, mesh->diameter(), 0.00001);
        mesh->normalize();
        assert_close(1.0, mesh->diameter(), 0.00001);
    }

    void test_user_data_works() {
        auto actor = stage_->new_actor();

        this->assert_true(actor->id()); //Make sure we set an id for the mesh
        this->assert_true(actor->auto_id() != 0); //Make sure we set a unique ID for the object
        this->assert_true(!actor->data->exists("data"));
        actor->data->stash((int)0xDEADBEEF, "data");
        this->assert_true(actor->data->exists("data"));
        this->assert_equal((int)0xDEADBEEF, actor->data->get<int>("data"));

        auto id = actor->id();
        stage_->destroy_actor(actor->id());
        window->run_frame();

        this->assert_true(!stage_->has_actor(id));
    }

    void test_deleting_entities_deletes_children() {
        auto m = stage_->new_actor(); //Create the root mesh
        auto c1 = stage_->new_actor_with_parent(m->id()); //Create a child
        auto c2 = stage_->new_actor_with_parent(c1->id()); //Create a child of the child

        auto mid = m->id();
        auto cid1 = c1->id();
        auto cid2 = c2->id();

        this->assert_equal((uint32_t)1, m->child_count());
        this->assert_equal((uint32_t)1, c1->child_count());
        this->assert_equal((uint32_t)0, c2->child_count());

        stage_->destroy_actor(mid);
        window->run_frame();

        this->assert_true(!stage_->has_actor(mid));
        this->assert_true(!stage_->has_actor(cid1));
        this->assert_true(!stage_->has_actor(cid2));
    }

    void test_basic_usage() {
        auto mesh = stage_->assets->mesh(generate_test_mesh(stage_));

        auto& data = mesh->vertex_data;

        assert_true(data->vertex_specification().has_positions());
        assert_true(!data->vertex_specification().has_normals());
        assert_true(!data->vertex_specification().has_texcoord0());
        assert_true(!data->vertex_specification().has_texcoord1());
        assert_true(!data->vertex_specification().has_texcoord2());
        assert_true(!data->vertex_specification().has_texcoord3());
        assert_true(!data->vertex_specification().has_diffuse());
        assert_true(!data->vertex_specification().has_specular());
        assert_equal(4u, data->count());

        assert_equal(2u, mesh->submesh_count());
    }

    void test_actor_from_mesh() {
        auto mesh = stage_->assets->mesh(generate_test_mesh(stage_));

        auto actor = stage_->new_actor();

        assert_true(!actor->has_any_mesh());

        actor->set_mesh(mesh->id());

        assert_true(actor->has_any_mesh());
        assert_true(actor->has_mesh(DETAIL_LEVEL_NEAREST));
        assert_false(actor->has_mesh(DETAIL_LEVEL_NEAR));
        assert_false(actor->has_mesh(DETAIL_LEVEL_MID));
        assert_false(actor->has_mesh(DETAIL_LEVEL_FAR));
        assert_false(actor->has_mesh(DETAIL_LEVEL_FARTHEST));

        //The actor's MeshID should match the mesh we set
        assert_true(mesh->id() == actor->mesh(DETAIL_LEVEL_NEAREST)->id());

        assert_equal(actor->aabb().min(), mesh->aabb().min());
        assert_equal(actor->aabb().max(), mesh->aabb().max());
    }

    void test_scene_methods() {
        smlt::MeshID mesh_id = stage_->assets->new_mesh(smlt::VertexSpecification::POSITION_ONLY); //Create a mesh
        auto actor = stage_->new_actor_with_mesh(mesh_id);

        assert_true(mesh_id == actor->mesh(DETAIL_LEVEL_NEAREST)->id());
    }

    // Skipped, currently fails
    void X_test_cubic_texture_generation() {
        auto mesh = stage_->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_box("cubic", stage_->assets->new_material(), 10.0f, 10.0f, 10.0f);
        stage_->assets->mesh(mesh)->first_submesh()->generate_texture_coordinates_cube();

        auto& vd = *stage_->assets->mesh(mesh)->first_submesh()->vertex_data.get();

        // Neg Z
        assert_equal(smlt::Vec2((1.0 / 3.0), 0), *vd.texcoord0_at<smlt::Vec2>(0));
        assert_equal(smlt::Vec2((2.0 / 3.0), 0), *vd.texcoord0_at<smlt::Vec2>(1));
        assert_equal(smlt::Vec2((2.0 / 3.0), (1.0 / 4.0)), *vd.texcoord0_at<smlt::Vec2>(2));
        assert_equal(smlt::Vec2((1.0 / 3.0), (1.0 / 4.0)), *vd.texcoord0_at<smlt::Vec2>(3));

        // Pos Z
        assert_equal(smlt::Vec2((1.0 / 3.0), (2.0 / 4.0)), *vd.texcoord0_at<smlt::Vec2>(4));
        assert_equal(smlt::Vec2((2.0 / 3.0), (2.0 / 4.0)), *vd.texcoord0_at<smlt::Vec2>(5));
        assert_equal(smlt::Vec2((2.0 / 3.0), (3.0 / 4.0)), *vd.texcoord0_at<smlt::Vec2>(6));
        assert_equal(smlt::Vec2((1.0 / 3.0), (3.0 / 4.0)), *vd.texcoord0_at<smlt::Vec2>(7));

        // Neg X
        assert_equal(smlt::Vec2(0, 2.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(8));
        assert_equal(smlt::Vec2(1.0 / 3.0, 2.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(9));
        assert_equal(smlt::Vec2(1.0 / 3.0, 3.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(10));
        assert_equal(smlt::Vec2(0, 3.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(11));

        // Pos X
        assert_equal(smlt::Vec2(2.0 / 3.0, 2.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(12));
        assert_equal(smlt::Vec2(3.0 / 3.0, 2.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(13));
        assert_equal(smlt::Vec2(3.0 / 3.0, 3.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(14));
        assert_equal(smlt::Vec2(2.0 / 3.0, 3.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(15));

        // Neg Y
        assert_equal(smlt::Vec2(1.0 / 3.0, 1.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(16));
        assert_equal(smlt::Vec2(2.0 / 3.0, 1.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(17));
        assert_equal(smlt::Vec2(2.0 / 3.0, 2.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(18));
        assert_equal(smlt::Vec2(1.0 / 3.0, 2.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(19));

        // Pos Y
        assert_equal(smlt::Vec2(1.0 / 3.0, 3.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(20));
        assert_equal(smlt::Vec2(2.0 / 3.0, 3.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(21));
        assert_equal(smlt::Vec2(2.0 / 3.0, 4.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(22));
        assert_equal(smlt::Vec2(1.0 / 3.0, 4.0 / 4.0), *vd.texcoord0_at<smlt::Vec2>(23));
    }

private:
    smlt::CameraPtr camera_;
    smlt::StagePtr stage_;

    smlt::SubMesh* first_mesh_;
};

}

#endif // TEST_MESH_H
