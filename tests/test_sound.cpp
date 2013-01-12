#include <unittest++/UnitTest++.h>
#include <kglt/kglt.h>

SUITE(sound) {
    TEST(test_2d_sound_output) {

        kglt::Window::ptr window = kglt::Window::create();
        kglt::SubScene& subscene = window->scene().subscene();

        kglt::Sound& sound = subscene.sound(subscene.new_sound());
        window->loader_for("sample_data/test_sound.mp3")->into(sound);

        CHECK(!subscene.camera().is_playing_sound());

        subscene.camera().play(sound.id());

        CHECK(subscene.camera().is_playing_sound());
    }

    TEST(test_3d_sound_output) {
        kglt::Window::ptr window = kglt::Window::create();
        kglt::SubScene& subscene = window->scene().subscene();

        kglt::Sound& sound = subscene.sound(subscene.new_sound());
        window->loader_for("sample_data/test_sound.mp3")->into(sound);

        kglt::Entity& entity = subscene.entity(subscene.new_entity());
        entity.move_to(10, 0, 0);

        CHECK(!entity.is_playing_sound());

        entity.play(sound.id());

        CHECK(entity.is_playing_sound());
    }
}
