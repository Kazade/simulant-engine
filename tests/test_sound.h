#ifndef TEST_SOUND_H
#define TEST_SOUND_H

#include "kglt/kglt.h"
#include "kaztest/kaztest.h"

#include "global.h"

class SoundTest : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();
        camera_id_ = window->new_camera();
        stage_id_ = window->new_stage();
    }

    void tear_down() {
        KGLTTestCase::tear_down();
        window->delete_camera(camera_id_);
        window->delete_stage(stage_id_);
    }

    void test_2d_sound_output() {
        kglt::SoundID sound = window->shared_assets->new_sound_from_file("sample_data/test_sound.ogg");

        assert_false(window->playing_sound_count());

        window->play_sound(sound);

        assert_true(window->playing_sound_count());

        while(window->playing_sound_count()) {
            window->run_frame();
        }
    }

    void test_3d_sound_output() {
        auto stage = window->stage(stage_id_);

        kglt::SoundID sound = stage->assets->new_sound_from_file("sample_data/test_sound.ogg");

        auto actor = stage->actor(stage->new_actor());
        actor->move_to(10, 0, 0);

        assert_false(actor->playing_sound_count());

        actor->play_sound(sound);

        assert_true(actor->playing_sound_count());
    }

private:
    kglt::CameraID camera_id_;
    kglt::StageID stage_id_;

};
#endif // TEST_SOUND_H
