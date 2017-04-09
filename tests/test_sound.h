#ifndef TEST_SOUND_H
#define TEST_SOUND_H

#include <cstdlib>
#include "simulant/simulant.h"
#include "kaztest/kaztest.h"

#include "global.h"

class SoundTest : public SimulantTestCase {
public:
    void set_up() {
	skip_if(smlt::Sound::is_disabled(), "No sound devices");

#ifdef __APPLE__
	bool skip = bool(std::getenv("TRAVIS"));
	skip_if(skip, "OSX Travis builds hang on sound tests :(");
#endif

        SimulantTestCase::set_up();
        camera_id_ = window->new_camera();
        stage_id_ = window->new_stage();
    }

    void tear_down() {
        SimulantTestCase::tear_down();
        window->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
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

    void test_3d_sound_output() {
        auto stage = window->stage(stage_id_);

        smlt::SoundID sound = stage->assets->new_sound_from_file("test_sound.ogg");

        auto actor = stage->actor(stage->new_actor());
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
    smlt::CameraID camera_id_;
    smlt::StageID stage_id_;

};
#endif // TEST_SOUND_H
