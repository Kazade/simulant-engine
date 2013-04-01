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

        kglt::Sound& sound = subscene.sound(subscene.new_sound());
        window->loader_for("sample_data/test_sound.ogg")->into(sound);

        assert_false(subscene.camera().is_playing_sound());

        subscene.camera().play(sound.id());

        assert_true(subscene.camera().is_playing_sound());

        while(subscene.camera().is_playing_sound()) {
            subscene.camera().update(0.1);
        }
    }

    void test_3d_sound_output() {
        kglt::SubScene& subscene = window->scene().subscene();

        kglt::Sound& sound = subscene.sound(subscene.new_sound());
        window->loader_for("sample_data/test_sound.ogg")->into(sound);

        kglt::Entity& entity = subscene.entity(subscene.new_entity());
        entity.move_to(10, 0, 0);

        assert_false(entity.is_playing_sound());

        entity.play(sound.id());

        assert_true(entity.is_playing_sound());
    }

};
#endif // TEST_SOUND_H
