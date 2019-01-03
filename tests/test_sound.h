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

        stage_ = window->new_stage();
        camera_ = stage_->new_camera();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        window->delete_stage(stage_->id());
    }

    void test_2d_sound_output() {
        smlt::SoundID sound = window->shared_assets->new_sound_from_file("test_sound.ogg");

        assert_false(window->playing_sound_count());

        window->play_sound(sound);

        assert_true(window->playing_sound_count());

        while(window->playing_sound_count()) {
            window->run_frame();
        }
    }

    void test_setting_audio_listener() {
        assert_is_null(window->audio_listener()); // No listener by default

        auto actor = stage_->new_actor();
        window->set_audio_listener(actor);

        assert_equal(window->audio_listener(), actor);

        stage_->delete_actor(actor->id());
        actor = nullptr;

        assert_is_null(window->audio_listener()); // Should now be NULL

        actor = stage_->new_actor();
        window->set_audio_listener(actor);

        assert_equal(window->audio_listener(), actor);
        window->delete_stage(stage_->id());

        assert_is_null(window->audio_listener());
    }

    void test_3d_sound_output() {
        smlt::SoundID sound = stage_->assets->new_sound_from_file("test_sound.ogg");

        auto actor = stage_->new_actor();
        actor->move_to(10, 0, 0);

        assert_false(actor->playing_sound_count());

        actor->play_sound(sound);

        assert_true(actor->playing_sound_count());

        // Finish playing the sound
        while(window->playing_sound_count()) {
            window->run_frame();
        }
    }

private:
    smlt::CameraPtr camera_;
    smlt::StagePtr stage_;

};
#endif // TEST_SOUND_H
