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

        assert_false(stage.camera().is_playing_sound());

        stage.camera().attach_sound(sound);
        stage.camera().play_sound();

        assert_true(stage.camera().is_playing_sound());

        while(stage.camera().is_playing_sound()) {
            stage.camera().update(0.1);
        }
    }

    void test_3d_sound_output() {
        kglt::Stage& stage = window->scene().stage();

        kglt::SoundID sound = stage.new_sound_from_file("sample_data/test_sound.ogg");

        kglt::Entity& entity = stage.entity(stage.new_entity());
        entity.move_to(10, 0, 0);

        assert_false(entity.is_playing_sound());

        entity.attach_sound(sound);
        entity.play_sound();

        assert_true(entity.is_playing_sound());
    }

};
#endif // TEST_SOUND_H
