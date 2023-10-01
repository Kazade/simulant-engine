#ifndef TEST_SOUND_H
#define TEST_SOUND_H

#include <cstdlib>
#include "simulant/simulant.h"
#include "simulant/test.h"


class SoundTest : public smlt::test::SimulantTestCase {
public:
    void set_up() {
#ifdef __APPLE__
    bool skip = bool(std::getenv("TRAVIS"));
    skip_if(skip, "OSX Travis builds hang on sound tests :(");
#endif

        SimulantTestCase::set_up();

        stage_ = scene->create_node<smlt::Stage>();
        camera_ = scene->create_node<smlt::Camera3D>();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        stage_->destroy();
        camera_->destroy();
    }

    void test_audio_listener() {
        assert_false(window->has_explicit_audio_listener());
        assert_is_null(window->audio_listener());

        auto p = window->compositor->render(scene, camera_);
        p->activate();

        // Make the first camera of the first pipeline the audio listener
        assert_equal(window->audio_listener(), camera_);
        assert_false(window->has_explicit_audio_listener());

        auto actor = scene->create_node<smlt::Stage>();
        window->set_audio_listener(actor);

        assert_equal(window->audio_listener(), actor);
        assert_true(window->has_explicit_audio_listener());

        actor->destroy();
        application->run_frame(); // actually destroy

        assert_equal(window->audio_listener(), camera_);
        assert_false(window->has_explicit_audio_listener());
    }

    void test_global_output() {
        smlt::SoundPtr sound = application->shared_assets->new_sound_from_file("test_sound.ogg");
        auto playing = application->sound_driver->play_sound(sound);
        assert_true(playing->is_playing());
        while(playing->is_playing()) {
            application->run_frame();
        }
    }

    void test_2d_sound_output() {
        smlt::SoundPtr sound = application->shared_assets->new_sound_from_file("test_sound.ogg");

        auto actor = scene->create_node<smlt::Stage>();

        assert_false(actor->playing_sound_count());

        actor->play_sound(sound);

        assert_true(actor->playing_sound_count());

        while(actor->playing_sound_count()) {
            application->run_frame();
        }
    }

    void test_played_signal() {
        bool played = false;

        smlt::SoundPtr sound = application->shared_assets->new_sound_from_file("test_sound.ogg");

        auto actor = scene->create_node<smlt::Stage>();

        assert_false(actor->playing_sound_count());

        actor->signal_sound_played().connect([&](smlt::SoundPtr s, smlt::AudioRepeat repeat, smlt::DistanceModel model) {
            played = true;

            /* Check args to the signal are correct */
            assert_equal(s, sound);
            assert_equal(repeat, smlt::AUDIO_REPEAT_NONE);
            assert_equal(model, smlt::DISTANCE_MODEL_DEFAULT);
        });

        actor->play_sound(sound);

        assert_true(actor->playing_sound_count());
        assert_true(played);  /* Signal should have fired */

        while(actor->playing_sound_count()) {
            application->run_frame();
        }
    }

    void test_3d_sound_output() {
        smlt::SoundPtr sound = scene->assets->new_sound_from_file("test_sound.ogg");

        auto actor = scene->create_node<smlt::Stage>();
        actor->transform->set_translation(smlt::Vec3(10, 0, 0));

        assert_false(actor->playing_sound_count());

        actor->play_sound(sound);

        assert_true(actor->playing_sound_count());

        // Finish playing the sound
        while(actor->playing_sound_count()) {
            application->run_frame();
        }
    }

    void test_sound_destruction_stops_play() {
        auto sound = application->shared_assets->new_sound_from_file("test_sound.ogg");

        auto sid = sound->id();

        auto a = scene->create_node<smlt::Stage>();

        assert_true(application->shared_assets->has_sound(sid));
        a->play_sound(sound);

        assert_true(a->is_sound_playing());

        application->shared_assets->destroy_sound(sound->id());
        sound.reset();

        application->shared_assets->run_garbage_collection();
        while(a->playing_sound_count()) {
            application->run_frame();
        }

        assert_false(application->shared_assets->has_sound(sid));
        assert_false(a->is_sound_playing());
    }

    void test_sound_stopping() {
        auto sound = application->shared_assets->new_sound_from_file("test_sound.ogg");
        auto a = scene->create_node<smlt::Stage>();
        smlt::PlayingSoundPtr s = a->play_sound(sound);

        assert_true(s);
        assert_true(s->id()); // id > 0
        assert_true(a->is_sound_playing());
        assert_true(a->stop_sound(s->id()));
        assert_false(a->is_sound_playing());
    }

private:
    smlt::CameraPtr camera_;
    smlt::StagePtr stage_;

};
#endif // TEST_SOUND_H
