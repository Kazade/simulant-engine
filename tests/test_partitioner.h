#pragma once

#include <functional>
#include "simulant/test.h"

#include "simulant/partitioner.h"
#include "simulant/stage.h"
#include "simulant/nodes/actor.h"
#include "simulant/nodes/particle_system.h"


namespace {

using namespace smlt;

class MockPartitioner : public Partitioner {
public:
    MockPartitioner(StagePtr stage, std::function<void (const StagedWrite&)> cb):
        Partitioner(stage),
        cb_(cb) {}

    void apply_staged_write(const StagedWrite& write) {
        cb_(write);
    }

    void lights_and_geometry_visible_from(CameraID camera_id, std::vector<LightID> &lights_out, std::vector<StageNode*> &geom_out) {

    }

private:
    std::function<void (const StagedWrite&)> cb_;
};


class PartitionerTests : public smlt::test::SimulantTestCase {
public:
    void test_add_actor_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_ACTOR);
            assert_equal(write.operation, WRITE_OPERATION_ADD);
            assert_true(write.actor_id);
            assert_false(write.light_id);
            assert_false(write.particle_system_id);
            assert_false(write.geom_id);
        };

        StagePtr stage = window->new_stage();
        ActorPtr actor = stage->new_actor();

        MockPartitioner partitioner(stage, test);
        partitioner.add_actor(actor->id());
        partitioner._apply_writes();

        window->destroy_stage(stage->id());
    }

    void test_remove_actor_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_ACTOR);
            assert_equal(write.operation, WRITE_OPERATION_REMOVE);
            assert_true(write.actor_id);
            assert_false(write.light_id);
            assert_false(write.particle_system_id);
            assert_false(write.geom_id);
        };

        StagePtr stage = window->new_stage();
        ActorPtr actor = stage->new_actor();

        MockPartitioner partitioner(stage, test);
        partitioner.remove_actor(actor->id());
        partitioner._apply_writes();

        window->destroy_stage(stage->id());
    }

    void test_add_light_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_LIGHT);
            assert_equal(write.operation, WRITE_OPERATION_ADD);
            assert_false(write.actor_id);
            assert_true(write.light_id);
            assert_false(write.particle_system_id);
            assert_false(write.geom_id);
        };

        StagePtr stage = window->new_stage();
        auto light = stage->new_light_as_point();

        MockPartitioner partitioner(stage, test);
        partitioner.add_light(light->id());
        partitioner._apply_writes();

        window->destroy_stage(stage->id());
    }

    void test_remove_light_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_LIGHT);
            assert_equal(write.operation, WRITE_OPERATION_REMOVE);
            assert_false(write.actor_id);
            assert_true(write.light_id);
            assert_false(write.particle_system_id);
            assert_false(write.geom_id);
        };

        StagePtr stage = window->new_stage();
        auto light = stage->new_light_as_point();

        MockPartitioner partitioner(stage, test);
        partitioner.remove_light(light->id());
        partitioner._apply_writes();

        window->destroy_stage(stage->id());
    }

    void test_add_particle_system_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_PARTICLE_SYSTEM);
            assert_equal(write.operation, WRITE_OPERATION_ADD);
            assert_false(write.actor_id);
            assert_false(write.light_id);
            assert_true(write.particle_system_id);
            assert_false(write.geom_id);
        };

        StagePtr stage = window->new_stage();
        auto ps = stage->new_particle_system();

        MockPartitioner partitioner(stage, test);
        partitioner.add_particle_system(ps->id());
        partitioner._apply_writes();

        window->destroy_stage(stage->id());
    }

    void test_remove_particle_system_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_PARTICLE_SYSTEM);
            assert_equal(write.operation, WRITE_OPERATION_REMOVE);
            assert_false(write.actor_id);
            assert_false(write.light_id);
            assert_true(write.particle_system_id);
            assert_false(write.geom_id);
        };

        StagePtr stage = window->new_stage();
        auto ps = stage->new_particle_system();

        MockPartitioner partitioner(stage, test);
        partitioner.remove_particle_system(ps->id());
        partitioner._apply_writes();
        window->destroy_stage(stage->id());
    }
};

}
