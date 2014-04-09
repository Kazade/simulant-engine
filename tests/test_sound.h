#ifndef TEST_SOUND_H
#define TEST_SOUND_H

#include "kglt/kglt.h"
#include <kaztest/kaztest.h>

#include "global.h"

class SoundTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_2d_sound_output() {
        auto stage = window->scene().stage();

        kglt::SoundID sound = stage->new_sound_from_file("sample_data/test_sound.ogg");

        assert_false(window->playing_sound_count());

        window->play_sound(sound);

        assert_true(window->playing_sound_count());

        while(window->playing_sound_count()) {
            window->run_frame();
        }
    }

    void test_3d_sound_output() {
        auto stage = window->scene().stage();

        kglt::SoundID sound = stage->new_sound_from_file("sample_data/test_sound.ogg");

        auto actor = stage->actor(stage->new_actor());
        actor->move_to(10, 0, 0);

        assert_false(actor->playing_sound_count());

        actor->play_sound(sound);

        assert_true(actor->playing_sound_count());
    }

};
#endif // TEST_SOUND_H
