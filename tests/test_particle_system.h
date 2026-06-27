#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/assets/particles/direction_manipulator.h"

namespace {

using namespace smlt;


class ParticleSystemTests : public test::SimulantTestCase {
public:
    void test_destroy_signal() {
        ParticleScriptPtr script = scene->assets->load_particle_script(
            ParticleScript::BuiltIns::FIRE
        );
        ParticleSystemPtr system = scene->create_child<ParticleSystem>(script);

        bool fired = false;
        system->signal_destroyed().connect([&]() {
            fired = true;
        });

        assert_false(fired);
        system->destroy();
        assert_true(fired);
    }

    void test_emitter_duration() {
        ParticleScriptPtr script = scene->assets->load_particle_script(
            ParticleScript::BuiltIns::FIRE
        );

        /* Disable repeat delay and fix duration */
        auto emitter = script->mutable_emitter(0);

        emitter->repeat_delay_range.first = 0;
        emitter->repeat_delay_range.second = 0;
        emitter->duration_range.first = 5.0f;
        emitter->duration_range.second = 5.0f;

        ParticleSystemPtr system = scene->create_child<ParticleSystem>(script);

        assert_true(system->has_active_emitters());
        system->update(1.0f);

        assert_true(system->has_active_emitters());
        system->update(4.1f);

        assert_false(system->has_active_emitters());
    }

    void test_world_space() {
        ParticleScriptPtr script =
            scene->assets->load_particle_script(ParticleScript::BuiltIns::FIRE);

        ParticleSystemPtr system = scene->create_child<ParticleSystem>(script);
        system->update(0.1f);
        assert_true(system->particle_count() > 0);

        // Move the system above the particles, the particles
        system->transform->set_position(smlt::Vec3(0, 100, 0));

        system->update(0.1f);
        auto p0 = system->particle(0);

        assert_true(p0.position.y < 100);
    }

    void test_local_space() {
        ParticleScriptPtr script =
            scene->assets->load_particle_script(ParticleScript::BuiltIns::FIRE);

        ParticleSystemPtr system = scene->create_child<ParticleSystem>(script);
        system->set_space(smlt::PARTICLE_SYSTEM_SPACE_LOCAL);

        system->update(0.1f);
        assert_true(system->particle_count() > 0);

        system->transform->set_position(smlt::Vec3(0, 100, 0));

        system->update(0.1f);
        auto p0 = system->particle(0);

        // Should be a small value
        assert_true(p0.position.y <= p0.velocity.y * 0.1f);
    }

    void test_direction_manipulator() {
        ParticleScriptPtr script = scene->assets->load_particle_script(
            ParticleScript::BuiltIns::FIRE
        );

        script->add_manipulator(std::make_shared<DirectionManipulator>(
            script.get(), smlt::Vec3::down()));

        ParticleSystemPtr system = scene->create_child<ParticleSystem>(script);
        system->update(0.1f);
        assert_true(system->particle_count() > 0);
        auto p0 = system->particle(0);
        system->update(0.1f);
        auto p1 = system->particle(0);

        assert_true(p1.position.y < p0.position.y);
    }

    void test_hex_color_parsing() {
        auto tmpdir = kfs::temp_dir();
        auto test_file = kfs::path::join(tmpdir, "hex_color_test.kglp");

        {
            std::ofstream out(test_file);
            out << R"({
                "name": "hex_test",
                "quota": 10,
                "emitters": [{
                    "emission_rate": 5,
                    "ttl": 1.0,
                    "direction": "0.0 1.0 0.0",
                    "color": "#FF8000"
                }]
            })";
        }

        auto vfs = application->vfs.get();
        vfs->add_search_path(tmpdir);

        auto script = scene->assets->load_particle_script("hex_color_test.kglp");
        assert_is_not_null(script.get());

        auto emitter = script->emitter(0);
        assert_is_not_null(emitter);
        assert_equal(emitter->colors.size(), 1u);

        auto c = emitter->colors[0];
        assert_close(c.r, 1.0f, 0.01f);
        assert_close(c.g, 0.502f, 0.01f);
        assert_close(c.b, 0.0f, 0.01f);
        assert_close(c.a, 1.0f, 0.01f);

        vfs->remove_search_path(tmpdir);
        std::remove(test_file.c_str());
    }

    void test_hex_color_lowercase() {
        auto tmpdir = kfs::temp_dir();
        auto test_file = kfs::path::join(tmpdir, "hex_lower_test.kglp");

        {
            std::ofstream out(test_file);
            out << R"({
                "name": "hex_lower_test",
                "quota": 10,
                "emitters": [{
                    "emission_rate": 5,
                    "ttl": 1.0,
                    "direction": "0.0 1.0 0.0",
                    "color": "#00ff80"
                }]
            })";
        }

        auto vfs = application->vfs.get();
        vfs->add_search_path(tmpdir);

        auto script = scene->assets->load_particle_script("hex_lower_test.kglp");
        assert_is_not_null(script.get());

        auto emitter = script->emitter(0);
        auto c = emitter->colors[0];
        assert_close(c.r, 0.0f, 0.01f);
        assert_close(c.g, 1.0f, 0.01f);
        assert_close(c.b, 0.502f, 0.01f);
        assert_close(c.a, 1.0f, 0.01f);

        vfs->remove_search_path(tmpdir);
        std::remove(test_file.c_str());
    }

    void test_hex_color_mixed_case() {
        auto tmpdir = kfs::temp_dir();
        auto test_file = kfs::path::join(tmpdir, "hex_mixed_test.kglp");

        {
            std::ofstream out(test_file);
            out << R"({
                "name": "hex_mixed_test",
                "quota": 10,
                "emitters": [{
                    "emission_rate": 5,
                    "ttl": 1.0,
                    "direction": "0.0 1.0 0.0",
                    "color": "#AbCDeF"
                }]
            })";
        }

        auto vfs = application->vfs.get();
        vfs->add_search_path(tmpdir);

        auto script = scene->assets->load_particle_script("hex_mixed_test.kglp");
        assert_is_not_null(script.get());

        auto emitter = script->emitter(0);
        auto c = emitter->colors[0];
        assert_close(c.r, 0.671f, 0.01f);
        assert_close(c.g, 0.804f, 0.01f);
        assert_close(c.b, 0.937f, 0.01f);
        assert_close(c.a, 1.0f, 0.01f);

        vfs->remove_search_path(tmpdir);
        std::remove(test_file.c_str());
    }

    void test_hex_color_with_alpha() {
        auto tmpdir = kfs::temp_dir();
        auto test_file = kfs::path::join(tmpdir, "hex_alpha_test.kglp");

        {
            std::ofstream out(test_file);
            out << R"({
                "name": "hex_alpha_test",
                "quota": 10,
                "emitters": [{
                    "emission_rate": 5,
                    "ttl": 1.0,
                    "direction": "0.0 1.0 0.0",
                    "color": "#FF800080"
                }]
            })";
        }

        auto vfs = application->vfs.get();
        vfs->add_search_path(tmpdir);

        auto script = scene->assets->load_particle_script("hex_alpha_test.kglp");
        assert_is_not_null(script.get());

        auto emitter = script->emitter(0);
        auto c = emitter->colors[0];
        assert_close(c.r, 1.0f, 0.01f);
        assert_close(c.g, 0.502f, 0.01f);
        assert_close(c.b, 0.0f, 0.01f);
        assert_close(c.a, 0.502f, 0.01f);

        vfs->remove_search_path(tmpdir);
        std::remove(test_file.c_str());
    }

    void test_hex_color_invalid_too_short() {
        auto tmpdir = kfs::temp_dir();
        auto test_file = kfs::path::join(tmpdir, "hex_short_test.kglp");

        {
            std::ofstream out(test_file);
            out << R"({
                "name": "hex_short_test",
                "quota": 10,
                "emitters": [{
                    "emission_rate": 5,
                    "ttl": 1.0,
                    "direction": "0.0 1.0 0.0",
                    "color": "#FFF"
                }]
            })";
        }

        auto vfs = application->vfs.get();
        vfs->add_search_path(tmpdir);

        auto script = scene->assets->load_particle_script("hex_short_test.kglp");
        assert_is_not_null(script.get());

        auto emitter = script->emitter(0);
        auto c = emitter->colors[0];
        // Should fall back to white
        assert_close(c.r, 1.0f, 0.01f);
        assert_close(c.g, 1.0f, 0.01f);
        assert_close(c.b, 1.0f, 0.01f);

        vfs->remove_search_path(tmpdir);
        std::remove(test_file.c_str());
    }

    void test_hex_color_invalid_no_hash() {
        auto tmpdir = kfs::temp_dir();
        auto test_file = kfs::path::join(tmpdir, "hex_nohash_test.kglp");

        {
            std::ofstream out(test_file);
            out << R"({
                "name": "hex_nohash_test",
                "quota": 10,
                "emitters": [{
                    "emission_rate": 5,
                    "ttl": 1.0,
                    "direction": "0.0 1.0 0.0",
                    "color": "FF8000"
                }]
            })";
        }

        auto vfs = application->vfs.get();
        vfs->add_search_path(tmpdir);

        auto script = scene->assets->load_particle_script("hex_nohash_test.kglp");
        assert_is_not_null(script.get());

        auto emitter = script->emitter(0);
        auto c = emitter->colors[0];
        // Without #, it's parsed as space-separated floats: "FF8000" has 1 part
        // but no # prefix, so it falls through to the else branch -> white
        assert_close(c.r, 1.0f, 0.01f);
        assert_close(c.g, 1.0f, 0.01f);
        assert_close(c.b, 1.0f, 0.01f);

        vfs->remove_search_path(tmpdir);
        std::remove(test_file.c_str());
    }
};

}
