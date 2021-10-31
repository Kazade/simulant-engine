#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;


class ParticleSystemTests : public test::SimulantTestCase {
public:
    void test_emitter_duration() {
        auto stage = new_stage();
        ParticleScriptPtr script = stage->assets->new_particle_script_from_file(
            ParticleScript::BuiltIns::FIRE
        );

        /* Disable repeat delay and fix duration */
        auto emitter = script->mutable_emitter(0);

        emitter->repeat_delay_range.first = 0;
        emitter->repeat_delay_range.second = 0;
        emitter->duration_range.first = 5.0f;
        emitter->duration_range.second = 5.0f;

        ParticleSystemPtr system = stage->new_particle_system(script);

        assert_true(system->has_active_emitters());
        system->update(1.0f);

        assert_true(system->has_active_emitters());
        system->update(4.1f);

        assert_false(system->has_active_emitters());
    }
};

}
