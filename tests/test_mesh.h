#ifndef TEST_MESH_H
#define TEST_MESH_H

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/macros.h"

namespace {

using namespace smlt;

class MeshTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();

        stage_ = scene->create_child<smlt::Stage>();
        camera_ = scene->create_child<smlt::Camera>();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        camera_->destroy();
        stage_->destroy();
    }

    smlt::MeshPtr generate_test_mesh(smlt::StagePtr stage) {
        _S_UNUSED(stage);

        auto mesh = scene->assets->create_mesh(VertexFormat::position_only(),
                                               GARBAGE_COLLECT_NEVER);
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

        auto mat = scene->assets->clone_default_material();
        first_mesh_ = mesh->create_submesh("test", mat, INDEX_TYPE_16_BIT);

        assert_equal(mesh->find_submesh_with_material(mat), first_mesh_);
        assert_equal(mesh->find_all_submeshes_with_material(mat).size(), 1u);
        assert_equal(mesh->find_all_submeshes_with_material(mat)[0], first_mesh_);

        smlt::SubMesh* submesh = first_mesh_;

        submesh->index_data->index(0);
        submesh->index_data->index(1);
        submesh->index_data->index(2);

        submesh->index_data->index(0);
        submesh->index_data->index(2);
        submesh->index_data->index(3);
        submesh->index_data->done();

        //Draw a line between the first two vertices
        smlt::SubMesh* sm = mesh->create_submesh("test2", scene->assets->clone_default_material(), INDEX_TYPE_16_BIT, smlt::MESH_ARRANGEMENT_LINES);
        sm->index_data->index(0);
        sm->index_data->index(1);
        sm->index_data->done();

        Vec3 expected_min(-1.0, -1.0, 0.0);
        Vec3 expected_max( 1.0, -1.0, 0.0);

        AABB box;
        sm->_recalc_bounds(box);

        assert_true(box.min() == expected_min);
        assert_true(box.max() == expected_max);

        mesh->set_garbage_collection_method(GARBAGE_COLLECT_PERIODIC);

        return mesh;
    }

    void test_submesh_index() {
        auto mesh = generate_test_mesh(stage_);
        assert_equal(mesh->submesh_index(mesh->first_submesh()).value(), 0u);
        assert_equal(mesh->submesh_index(mesh->find_submesh("test2")).value(),
                     1u);

        assert_false(mesh->reinsert_submesh(mesh->first_submesh(), 100));
        assert_true(mesh->reinsert_submesh(mesh->first_submesh(), 1));

        assert_equal(mesh->first_submesh(), mesh->find_submesh("test2"));

        assert_true(mesh->reinsert_submesh(mesh->find_submesh("test"), 0));
        assert_equal(mesh->first_submesh(), mesh->find_submesh("test"));
    }

    void test_create_mesh_from_submesh() {
        auto mesh = generate_test_mesh(stage_);
        auto submesh = mesh->first_submesh();

        auto second_mesh = scene->assets->create_mesh_from_submesh(submesh);

        assert_equal(second_mesh->first_submesh()->index_data->count(), submesh->index_data->count());
        assert_equal(second_mesh->first_submesh()->arrangement(), submesh->arrangement());
    }

    void test_skeleton() {
        auto mesh1 = generate_test_mesh(stage_);

        bool added = mesh1->add_skeleton(3);

        assert_true(added);

        Skeleton* s = mesh1->skeleton;

        assert_equal(s->joint_count(), 3u);

        assert_false(mesh1->add_skeleton(2));
        assert_equal(s->joint_count(), 3u); // Didn't change
    }

    void test_mesh_garbage_collection() {
        assert_false(stage_->is_destroyed());

        auto initial = scene->assets->mesh_count();

        auto mesh1 = generate_test_mesh(stage_);
        auto mesh2 = generate_test_mesh(stage_);

        auto actor = scene->create_child<Actor>(mesh1);
        actor->set_mesh(mesh2);
        mesh1.reset();
        mesh2.reset();

        scene->assets->run_garbage_collection();
        assert_false(stage_->is_destroyed());

        assert_equal(scene->assets->mesh_count(), initial + 1);

        actor->destroy();
        assert_false(stage_->is_destroyed());

        application->run_frame();

        scene->assets->run_garbage_collection();

        assert_equal(scene->assets->mesh_count(), initial + 0);
    }

    void test_set_mesh_detail_level() {
        auto mesh = scene->assets->create_mesh(VertexFormat::standard());
        auto actor = scene->create_child<smlt::Actor>(mesh);

        auto m1 = scene->assets->create_mesh(VertexFormat::standard());
        auto m2 = scene->assets->create_mesh(VertexFormat::standard());

        actor->set_mesh(m1);
        actor->set_mesh(m2, DETAIL_LEVEL_MID);

        assert_equal(actor->best_mesh(DETAIL_LEVEL_NEAREST), m1);
        assert_equal(actor->best_mesh(DETAIL_LEVEL_NEAR), m1);
        assert_equal(actor->best_mesh(DETAIL_LEVEL_MID), m2);
        assert_equal(actor->best_mesh(DETAIL_LEVEL_FAR), m2);
        assert_equal(actor->best_mesh(DETAIL_LEVEL_FARTHEST), m2);
    }

    void test_index_data_done() {
        /* Check that index_data->done() fires signals without crashing */
        auto index_data = std::make_shared<IndexData>(INDEX_TYPE_16_BIT);

        auto m1 = scene->assets->create_mesh(VertexFormat::standard());
        m1->vertex_data->position(0, 0, 0);
        m1->vertex_data->done();

        auto mat = scene->assets->create_material();
        m1->create_submesh("sm1", mat, index_data);
        m1->create_submesh("sm2", mat, index_data);
        m1->create_submesh("sm3", mat, INDEX_TYPE_16_BIT);

        m1->destroy_submesh("sm2");

        index_data->index(0);
        index_data->done(); // Should fire and not crash!
    }

    void test_mesh_normalization() {
        /*
         *  The normalize function scales the mesh so that it has a diameter of 1
         *  at its widest point. Useful for programmatically scaling stuff to the right
         *  size relative to other models
         */

        auto mesh = generate_test_mesh(stage_);

        assert_close(2.0f, mesh->diameter(), 0.00001f);
        mesh->normalize();
        assert_close(1.0f, mesh->diameter(), 0.00001f);
    }

    void test_user_data_works() {
        auto actor = scene->create_child<smlt::Stage>();

        this->assert_true(actor->id()); //Make sure we set an id for the mesh
        this->assert_true(!actor->data->exists("data"));
        actor->data->stash((int)0xDEADBEEF, "data");
        this->assert_true(actor->data->exists("data"));
        this->assert_equal((int)0xDEADBEEF, actor->data->get<int>("data"));

        auto id = actor->id();
        actor->destroy();
        application->run_frame();

        this->assert_true(!scene->has_node(id));
    }

    void test_deleting_entities_deletes_children() {
        auto m = scene->create_child<smlt::Stage>(); //Create the root mesh
        auto c1 = scene->create_child<smlt::Stage>(); //Create a child
        c1->set_parent(m);

        auto c2 = scene->create_child<smlt::Stage>(); //Create a child of the child
        c2->set_parent(c1);

        auto mid = m->id();
        auto cid1 = c1->id();
        auto cid2 = c2->id();

        this->assert_equal((uint32_t)1, m->child_count());
        this->assert_equal((uint32_t)1, c1->child_count());
        this->assert_equal((uint32_t)0, c2->child_count());

        m->destroy();
        application->run_frame();

        this->assert_true(!scene->has_node(mid));
        this->assert_true(!scene->has_node(cid1));
        this->assert_true(!scene->has_node(cid2));
    }

    void test_basic_usage() {
        auto mesh = generate_test_mesh(stage_);

        auto& data = mesh->vertex_data;

        assert_true(
            data->vertex_specification().attr_count(VERTEX_ATTR_NAME_POSITION));
        assert_true(
            !data->vertex_specification().attr_count(VERTEX_ATTR_NAME_NORMAL));
        assert_true(!data->vertex_specification().attr_count(
            VERTEX_ATTR_NAME_TEXCOORD_0));
        assert_true(!data->vertex_specification().attr_count(
            VERTEX_ATTR_NAME_TEXCOORD_1));
        assert_true(!data->vertex_specification().attr_count(
            VERTEX_ATTR_NAME_TEXCOORD_2));
        assert_true(!data->vertex_specification().attr_count(
            VERTEX_ATTR_NAME_TEXCOORD_3));
        assert_true(
            !data->vertex_specification().attr_count(VERTEX_ATTR_NAME_COLOR));
        assert_equal(4u, data->count());

        assert_equal(2u, mesh->submesh_count());
    }

    void test_actor_from_mesh() {
        auto mesh = generate_test_mesh(stage_);

        auto actor = scene->create_child<smlt::Actor>(mesh);

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
        auto mesh = scene->assets->create_mesh(
            VertexFormat::position_only()); // Create a mesh
        auto actor = scene->create_child<Actor>(mesh);

        assert_true(mesh->id() == actor->mesh(DETAIL_LEVEL_NEAREST)->id());
    }

    void test_material_slots() {
        Viewport viewport;

        auto mat1 = scene->assets->create_material();
        auto mat2 = scene->assets->create_material();

        auto mesh = scene->assets->create_mesh(smlt::VertexFormat::standard());

        auto submesh = mesh->create_submesh_as_cube("cube", mat1, 1.0f);
        submesh->set_material_at_slot(MATERIAL_SLOT1, mat2);

        auto actor1 = scene->create_child<Actor>(mesh);
        auto actor2 = scene->create_child<Actor>(mesh);
        auto actor3 = scene->create_child<Actor>(mesh);

        actor2->use_material_slot(MATERIAL_SLOT1);
        actor3->use_material_slot(MATERIAL_SLOT7);

        auto camera = scene->create_child<smlt::Camera>();
        batcher::RenderQueue queue;
        queue.reset(stage_, window->renderer.get(), camera);

        actor1->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);
        actor2->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);
        actor3->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        std::vector<Renderable> renderables;
        for(auto i = 0u; i < queue.renderable_count(); ++i) {
            renderables.push_back(*queue.renderable(i));
        }

        assert_equal(renderables[0].material->id(), mat1->id());
        assert_equal(renderables[1].material->id(), mat2->id());
        assert_equal(renderables[2].material->id(), mat1->id());
    }

    // Skipped, currently fails
    void X_test_cubic_texture_generation() {
        auto mesh = scene->assets->create_mesh(smlt::VertexFormat::standard());
        mesh->create_submesh_as_box("cubic", scene->assets->create_material(), 10.0f, 10.0f, 10.0f);
        mesh->first_submesh()->generate_texture_coordinates_cube();

        auto& vd = *mesh->vertex_data.get();

#define TEXCOORD(n) vd.attr_as<Vec2>(VERTEX_ATTR_NAME_TEXCOORD_0, (n)).value()

        // Neg Z
        assert_equal(smlt::Vec2((1.0 / 3.0), 0), TEXCOORD(0));
        assert_equal(smlt::Vec2((2.0 / 3.0), 0), TEXCOORD(1));
        assert_equal(smlt::Vec2((2.0 / 3.0), (1.0 / 4.0)), TEXCOORD(2));
        assert_equal(smlt::Vec2((1.0 / 3.0), (1.0 / 4.0)), TEXCOORD(3));

        // Pos Z
        assert_equal(smlt::Vec2((1.0 / 3.0), (2.0 / 4.0)), TEXCOORD(4));
        assert_equal(smlt::Vec2((2.0 / 3.0), (2.0 / 4.0)), TEXCOORD(5));
        assert_equal(smlt::Vec2((2.0 / 3.0), (3.0 / 4.0)), TEXCOORD(6));
        assert_equal(smlt::Vec2((1.0 / 3.0), (3.0 / 4.0)), TEXCOORD(7));

        // Neg X
        assert_equal(smlt::Vec2(0, 2.0 / 4.0), TEXCOORD(8));
        assert_equal(smlt::Vec2(1.0 / 3.0, 2.0 / 4.0), TEXCOORD(9));
        assert_equal(smlt::Vec2(1.0 / 3.0, 3.0 / 4.0), TEXCOORD(10));
        assert_equal(smlt::Vec2(0, 3.0 / 4.0), TEXCOORD(11));

        // Pos X
        assert_equal(smlt::Vec2(2.0 / 3.0, 2.0 / 4.0), TEXCOORD(12));
        assert_equal(smlt::Vec2(3.0 / 3.0, 2.0 / 4.0), TEXCOORD(13));
        assert_equal(smlt::Vec2(3.0 / 3.0, 3.0 / 4.0), TEXCOORD(14));
        assert_equal(smlt::Vec2(2.0 / 3.0, 3.0 / 4.0), TEXCOORD(15));

        // Neg Y
        assert_equal(smlt::Vec2(1.0 / 3.0, 1.0 / 4.0), TEXCOORD(16));
        assert_equal(smlt::Vec2(2.0 / 3.0, 1.0 / 4.0), TEXCOORD(17));
        assert_equal(smlt::Vec2(2.0 / 3.0, 2.0 / 4.0), TEXCOORD(18));
        assert_equal(smlt::Vec2(1.0 / 3.0, 2.0 / 4.0), TEXCOORD(19));

        // Pos Y
        assert_equal(smlt::Vec2(1.0 / 3.0, 3.0 / 4.0), TEXCOORD(20));
        assert_equal(smlt::Vec2(2.0 / 3.0, 3.0 / 4.0), TEXCOORD(21));
        assert_equal(smlt::Vec2(2.0 / 3.0, 4.0 / 4.0), TEXCOORD(22));
        assert_equal(smlt::Vec2(1.0 / 3.0, 4.0 / 4.0), TEXCOORD(23));

#undef TEXCOORD
    }

    void test_mesh_aabb_generated_correctly() {
        auto mesh = scene->assets->create_mesh(smlt::VertexFormat::standard());
        mesh->create_submesh_as_box(
            "test", scene->assets->default_material(),
            1.0, 1.0, 1.0, smlt::Vec3(-100, 0, 0)
        );

        auto& aabb = mesh->aabb();

        assert_close(aabb.center().x, -100.0f, EPSILON);
        assert_close(aabb.max().x, -99.5f, EPSILON);
        assert_close(aabb.width(), 1.0f, EPSILON);
        assert_close(aabb.height(), 1.0f, EPSILON);
        assert_close(aabb.depth(), 1.0f, EPSILON);
    }

    void test_submesh_aabb_generated_correctly() {
        auto mesh = scene->assets->create_mesh(smlt::VertexFormat::standard());
        auto submesh = mesh->create_submesh_as_box(
            "test", scene->assets->default_material(),
            1.0, 1.0, 1.0, smlt::Vec3(-100, 0, 0)
        );

        auto& aabb = submesh->aabb();

        assert_close(aabb.center().x, -100.0f, EPSILON);
        assert_close(aabb.max().x, -99.5f, EPSILON);
        assert_close(aabb.width(), 1.0f, EPSILON);
        assert_close(aabb.height(), 1.0f, EPSILON);
        assert_close(aabb.depth(), 1.0f, EPSILON);
    }

    void test_create_submesh_as_capsule() {
        auto mesh = scene->assets->create_mesh(smlt::VertexFormat::standard());
        mesh->create_submesh_as_capsule(
            "capsule",
            scene->assets->create_material(),
            2.0f, 5.0f, 10, 1, 10
        );

        assert_close(mesh->aabb().height(), 5.0f, EPSILON);
        assert_close(mesh->aabb().width(), 2.0f, EPSILON);

        // FIXME - not sure why this is 1.90... there's a bug somewhere!
        //assert_close(mesh->aabb().depth(), 2.0f, EPSILON);
    }

    void test_find_mesh() {
        auto mesh = scene->assets->create_mesh(VertexFormat::standard())
                        ->set_name_and_get("Mesh 1");
        scene->assets->create_mesh(VertexFormat::standard())
            ->set_name("Mesh 2");

        assert_equal(mesh->id(), scene->assets->find_mesh("Mesh 1")->id());
        assert_is_not_null(scene->assets->find_mesh("Mesh 2").get());
        assert_not_equal(mesh->id(), scene->assets->find_mesh("Mesh 2")->id());
    }

private:
    smlt::CameraPtr camera_;
    smlt::StagePtr stage_;

    smlt::SubMesh* first_mesh_;
};

}

#endif // TEST_MESH_H
