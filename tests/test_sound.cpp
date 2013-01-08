
SUITE(sound) {
    TEST(test_2d_sound_output) {

        kglt::Window::ptr window = kglt::Window::create();
        kglt::SubScene& subscene = window->scene().subscene();

        kglt::Sound& sound = subscene.sound(subscene.new_sound());
        window->loader_for("sample_data/test_sound.mp3")->into(sound);

        CHECK_EQUAL(0, subscene.sounds_playing_count());
        CHECK_EQUAL(0, subscene.camera().sounds_playing_count());
        subscene.camera().play_sound(sound.id());
        CHECK_EQUAL(1, subscene.camera().sounds_playing_count());
        CHECK_EQUAL(1, subscene.sounds_playing_count());

    }
}
