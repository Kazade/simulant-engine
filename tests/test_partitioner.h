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

    void apply_staged_write(const UniqueIDKey& k, const StagedWrite& write) {
        _S_UNUSED(k);

        cb_(write);
    }

    void lights_and_geometry_visible_from(CameraID camera_id, std::vector<LightID> &lights_out, std::vector<StageNode*> &geom_out) {
        _S_UNUSED(camera_id);
        _S_UNUSED(lights_out);
        _S_UNUSED(geom_out);
    }

private:
    std::function<void (const StagedWrite&)> cb_;
};


class PartitionerTests : public smlt::test::SimulantTestCase {
public:
    void test_remove_first() {
        /* Because IDs could be reused, we need to handle both cases
         * that a remove is staged before an add, and an add is staged
         * before a remove */

        bool exists = false;

        auto test = [&exists](const StagedWrite& write) {
            if(write.operation == WRITE_OPERATION_ADD) {
                exists = true;
            }

            if(write.operation == WRITE_OPERATION_REMOVE) {
                exists = false;
            }
        };

        StagePtr stage = new_stage();
        ActorPtr actor = stage->new_actor();

        MockPartitioner partitioner(stage, test);
        partitioner.remove_actor(actor);
        partitioner.add_actor(actor);
        partitioner._apply_writes();

        assert_true(exists);
        exists = false;

        partitioner.add_actor(actor);
        partitioner.remove_actor(actor);
        partitioner._apply_writes();
        assert_false(exists);
    }

    void test_add_actor_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_ACTOR);
            assert_equal(write.operation, WRITE_OPERATION_ADD);
        };

        StagePtr stage = new_stage();
        ActorPtr actor = stage->new_actor();

        MockPartitioner partitioner(stage, test);
        partitioner.add_actor(actor->id());
        partitioner._apply_writes();

        destroy_stage(stage->id());
    }

    void test_remove_actor_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_ACTOR);
            assert_equal(write.operation, WRITE_OPERATION_REMOVE);
        };

        StagePtr stage = new_stage();
        ActorPtr actor = stage->new_actor();

        MockPartitioner partitioner(stage, test);
        partitioner.remove_actor(actor->id());
        partitioner._apply_writes();

        destroy_stage(stage->id());
    }

    void test_add_light_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_LIGHT);
            assert_equal(write.operation, WRITE_OPERATION_ADD);
        };

        StagePtr stage = new_stage();
        auto light = stage->new_light_as_point();

        MockPartitioner partitioner(stage, test);
        partitioner.add_light(light->id());
        partitioner._apply_writes();

        destroy_stage(stage->id());
    }

    void test_remove_light_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_LIGHT);
            assert_equal(write.operation, WRITE_OPERATION_REMOVE);
        };

        StagePtr stage = new_stage();
        auto light = stage->new_light_as_point();

        MockPartitioner partitioner(stage, test);
        partitioner.remove_light(light->id());
        partitioner._apply_writes();

        destroy_stage(stage->id());
    }

    void test_add_particle_system_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_PARTICLE_SYSTEM);
            assert_equal(write.operation, WRITE_OPERATION_ADD);
        };

        StagePtr stage = new_stage();
        auto ps = stage->new_particle_system(
            stage->assets->new_particle_script_from_file(ParticleScript::BuiltIns::FIRE)
        );

        MockPartitioner partitioner(stage, test);
        partitioner.add_particle_system(ps->id());
        partitioner._apply_writes();

        destroy_stage(stage->id());
    }

    void test_remove_particle_system_stages_write() {
        auto test = [=](const StagedWrite& write) {
            assert_equal(write.stage_node_type, STAGE_NODE_TYPE_PARTICLE_SYSTEM);
            assert_equal(write.operation, WRITE_OPERATION_REMOVE);
        };

        StagePtr stage = new_stage();
        auto ps = stage->new_particle_system(
            stage->assets->new_particle_script_from_file(ParticleScript::BuiltIns::FIRE)
        );

        MockPartitioner partitioner(stage, test);
        partitioner.remove_particle_system(ps->id());
        partitioner._apply_writes();
        destroy_stage(stage->id());
    }
};

}
