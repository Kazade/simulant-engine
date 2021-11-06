#pragma once

#include <set>

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class StageTests : public smlt::test::SimulantTestCase {
public:

    void test_actor_destruction() {
        auto stage = scene->new_stage();
        auto destroyed_count = 0;

        std::set<ActorID> destroyed_ids;
        sig::scoped_connection conn = stage->signal_actor_destroyed().connect([&](smlt::ActorID id) {
            destroyed_count++;
            destroyed_ids.insert(id);
        });

        auto a1 = stage->new_actor();
        auto a2 = stage->new_actor_with_parent(a1);
        stage->new_actor_with_parent(a2);

        auto a2id = a2->id();
        a2->destroy();

        application->run_frame();

        assert_false(a1->child_count()); // No children now
        assert_equal(destroyed_count, 2); // Should've destroyed 2

        assert_true(destroyed_ids.count(a2id));


    }

    void test_camera_destruction() {
        auto stage = scene->new_stage();
        auto destroyed_count = 0;

        std::set<CameraID> destroyed_ids;
        sig::scoped_connection conn = stage->signal_camera_destroyed().connect([&](smlt::CameraID id) {
            destroyed_count++;
            destroyed_ids.insert(id);
        });

        auto a1 = stage->new_camera();
        auto a2 = stage->new_camera();
        a2->set_parent(a1);

        auto a3 = stage->new_camera();
        a3->set_parent(a2);

        auto a2id = a2->id();
        a2->destroy();

        application->run_frame();

        assert_false(a1->child_count()); // No children now
        assert_equal(destroyed_count, 2); // Should've destroyed 2

        assert_true(destroyed_ids.count(a2id));
    }

    void test_particle_system_destruction() {
        auto stage = scene->new_stage();
        auto destroyed_count = 0;

        std::set<ParticleSystemID> destroyed_ids;
        sig::scoped_connection conn = stage->signal_particle_system_destroyed().connect([&](smlt::ParticleSystemID id) {
            destroyed_count++;
            destroyed_ids.insert(id);
        });

        auto script = stage->assets->new_particle_script_from_file(ParticleScript::BuiltIns::FIRE);
        auto a1 = stage->new_particle_system(script);

        auto a2 = stage->new_particle_system(script);
        a2->set_parent(a1);

        auto a3 = stage->new_particle_system(script);
        a3->set_parent(a2);

        auto a2id = a2->id();
        a2->destroy();

        application->run_frame();

        assert_false(a1->child_count()); // No children now
        assert_equal(destroyed_count, 2); // Should've destroyed 2

        assert_true(destroyed_ids.count(a2id));
    }

    void test_stage_node_clean_up_signals() {
        auto stage = scene->new_stage();

        auto actor = stage->new_actor();

        bool cleaned_up = false;
        bool destroyed = false;

        sig::scoped_connection conn = actor->signal_destroyed().connect([&]() {
            destroyed = true;
        });

        sig::scoped_connection conn2 = actor->signal_cleaned_up().connect([&]() {
            cleaned_up = true;
        });

        actor->destroy();

        assert_true(destroyed);
        assert_false(cleaned_up);

        application->run_frame();

        assert_true(destroyed);
        assert_true(cleaned_up);
    }

    void test_iteration_types() {
        auto stage = scene->new_stage();

        for(auto& node: stage->each_child()) {
            node.destroy();
        }
        application->run_frame();

        /*
            stage-> o
                   / \
               a1 o   o a2
                 /   /
            c1  o   o c4
               / \
          c2  o   o  c3
        */

        auto a1 = stage->new_actor();
        auto a2 = stage->new_actor();
        auto c1 = stage->new_actor();
        auto c2 = stage->new_actor();
        auto c3 = stage->new_actor();
        auto c4 = stage->new_actor();
        c1->set_parent(a1);
        c2->set_parent(c1);
        c3->set_parent(c1);
        c4->set_parent(a2);

        std::multiset<smlt::StageNode*> found;
        std::multiset<smlt::StageNode*> expected;

        /* Should iterate a1, and a2 */
        for(auto& node: a1->each_sibling()) {
            found.insert(&node);
        }

        expected.insert(a2);

        assert_items_equal(found, expected);

        found.clear();
        expected.clear();

        /* All nodes */
        found.insert(stage);
        for(auto& node: stage->each_descendent()) {
            found.insert(&node);
        }

        expected.insert(stage);
        expected.insert(a1);
        expected.insert(a2);
        expected.insert(c1);
        expected.insert(c2);
        expected.insert(c3);
        expected.insert(c4);

        assert_items_equal(found, expected);

        found.clear();

        /* All nodes, leaf-first */
        found.insert(stage);
        for(auto& node: stage->each_descendent()) {
            found.insert(&node);
        }

        // FIXME: this doesn't test the order
        assert_items_equal(found, expected);
        found.clear();
        expected.clear();

        for(auto& node: c1->each_descendent()) {
            found.insert(&node);
        }
        expected.insert(c2);
        expected.insert(c3);
        assert_items_equal(found, expected);
        found.clear();
        expected.clear();

        /* a1 and a2 only */
        for(auto& node: stage->each_child()) {
            found.insert(&node);
        }

        expected.insert(a1);
        expected.insert(a2);

        assert_items_equal(found, expected);

        found.clear();
        expected.clear();

        for(auto& node: a1->each_descendent()) {
            found.insert(&node);
        }

        expected.insert(c1);
        expected.insert(c2);
        expected.insert(c3);

        assert_items_equal(found, expected);

        found.clear();
        for(auto& node: a1->each_descendent()) {
            found.insert(&node);
        }

        assert_items_equal(found, expected);

        found.clear();
        expected.clear();

        for(auto& node: c2->each_ancestor()) {
            found.insert(&node);
        }

        expected.insert(c1);
        expected.insert(a1);
        expected.insert(stage);

        assert_items_equal(found, expected);
    }
};

}
