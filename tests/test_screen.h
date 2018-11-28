#pragma once



namespace {

using namespace smlt;

class ScreenTests : public smlt::test::SimulantTestCase {

public:
    void test_registering() {
        uint8_t added = 0;

        auto conn = window->signal_screen_added().connect([&](std::string name, Screen* screen) {
            added++;
        });

        auto screen = window->_create_screen("Test1", 32, 32, SCREEN_FORMAT_G1, 60);

        assert_is_not_null(screen);
        assert_equal(added, 1);
        assert_equal(window->screen_count(), 1u);
        assert_equal(window->screen("Test1"), screen);

        conn.disconnect();

        window->_destroy_screen("Test1");

        assert_equal(window->screen_count(), 0u);
        assert_is_null(window->screen("Test1"));
    }
};


}
