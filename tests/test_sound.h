#ifndef TEST_SOUND_H
#define TEST_SOUND_H

#include "kglt/kglt.h"
#include "kglt/kazbase/testing.h"

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
        kglt::Stage& stage = window->scene().stage();

        kglt::SoundID sound = stage.new_sound_from_file("sample_data/test_sound.ogg");

        assert_false(window->is_playing_sound());

        window->attach_sound(sound);
        window->play_sound();

        assert_true(window->is_playing_sound());

        while(window->is_playing_sound()) {
            window->update();
        }
    }

    void test_3d_sound_output() {
        kglt::Stage& stage = window->scene().stage();

        kglt::SoundID sound = stage.new_sound_from_file("sample_data/test_sound.ogg");

        auto actor = stage.actor(stage.new_actor());
        actor->move_to(10, 0, 0);

        assert_false(actor->is_playing_sound());

        actor->attach_sound(sound);
        actor->play_sound();

        assert_true(actor->is_playing_sound());
    }

};
#endif // TEST_SOUND_H
