#ifndef TEST_VIRTUAL_GAMEPAD_H
#define TEST_VIRTUAL_GAMEPAD_H

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"

namespace {

using namespace kglt;

class VirtualGamepadTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        }

        window->reset();
    }

    void test_touchdown_event_triggers_signal() {

    }

    void test_touchup_event_triggers_signal() {

    }

    void test_second_touch_event_doesnt_interfere() {

    }

    void test_button_released_when_all_touches_are_finished() {

    }

};

}


#endif // TEST_VIRTUAL_GAMEPAD_H
