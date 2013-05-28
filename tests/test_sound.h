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
        kglt::SubScene& subscene = window->scene().subscene();

        kglt::SoundID sound = subscene.new_sound_from_file("sample_data/test_sound.ogg");

        assert_false(subscene.camera().is_playing_sound());

        subscene.camera().attach_sound(sound);
        subscene.camera().play_sound();

        assert_true(subscene.camera().is_playing_sound());

        while(subscene.camera().is_playing_sound()) {
            subscene.camera().update(0.1);
        }
    }

    void test_3d_sound_output() {
        kglt::SubScene& subscene = window->scene().subscene();

        kglt::SoundID sound = subscene.new_sound_from_file("sample_data/test_sound.ogg");

        kglt::Entity& entity = subscene.entity(subscene.new_entity());
        entity.move_to(10, 0, 0);

        assert_false(entity.is_playing_sound());

        entity.attach_sound(sound);
        entity.play_sound();

        assert_true(entity.is_playing_sound());
    }

};
#endif // TEST_SOUND_H
