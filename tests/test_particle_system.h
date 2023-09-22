#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/assets/particles/direction_manipulator.h"

namespace {

using namespace smlt;


class ParticleSystemTests : public test::SimulantTestCase {
public:
    void test_destroy_signal() {
        auto stage = scene->create_node<smlt::Stage>();
        ParticleScriptPtr script = scene->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );
        ParticleSystemPtr system = scene->create_node<ParticleSystem>(script);

        bool fired = false;
        system->signal_destroyed().connect([&]() {
            fired = true;
        });

        assert_false(fired);
        system->destroy();
        assert_true(fired);
    }

    void test_emitter_duration() {
        auto stage = scene->create_node<smlt::Stage>();
        ParticleScriptPtr script = scene->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );

        /* Disable repeat delay and fix duration */
        auto emitter = script->mutable_emitter(0);

        emitter->repeat_delay_range.first = 0;
        emitter->repeat_delay_range.second = 0;
        emitter->duration_range.first = 5.0f;
        emitter->duration_range.second = 5.0f;

        ParticleSystemPtr system = scene->create_node<ParticleSystem>(script);

        assert_true(system->has_active_emitters());
        system->update(1.0f);

        assert_true(system->has_active_emitters());
        system->update(4.1f);

        assert_false(system->has_active_emitters());
    }

    void test_direction_manipulator() {
        auto stage = scene->create_node<smlt::Stage>();
        ParticleScriptPtr script = scene->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );

        script->add_manipulator(
            std::make_shared<DirectionManipulator>(script.get(), smlt::Vec3::NEGATIVE_Y)
        );

        ParticleSystemPtr system = scene->create_node<ParticleSystem>(script);
        system->update(0.1f);
        assert_true(system->particle_count() > 0);
        auto p0 = system->particle(0);
        system->update(0.1f);
        auto p1 = system->particle(0);

        assert_true(p1.position.y < p0.position.y);
    }
};

}
