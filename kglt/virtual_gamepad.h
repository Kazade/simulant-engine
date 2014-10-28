#ifndef VIRTUAL_GAMEPAD_H
#define VIRTUAL_GAMEPAD_H

#include <kazbase/signals.h>
#include "generic/managed.h"
#include "types.h"

namespace kglt {

class VirtualGamepad : public Managed<VirtualGamepad> {
public:
    VirtualGamepad(WindowBase& window, VirtualDPadDirections directions, int button_count);

    bool init();
    void cleanup();

    void flip();

    sig::signal<void (int)>& signal_button_down() { return signal_button_down_; }
    sig::signal<void (int)>& signal_button_up() { return signal_button_up_; }

    sig::signal<void ()>& signal_left_down() { return signal_left_down_; }
    sig::signal<void ()>& signal_left_up() { return signal_left_up_; }

    sig::signal<void ()>& signal_right_down() { return signal_right_down_; }
    sig::signal<void ()>& signal_right_up() { return signal_right_up_; }

private:
    WindowBase& window_;
    VirtualDPadDirections directions_ = VIRTUAL_DPAD_DIRECTIONS_TWO;
    int button_count_ = 0;

    UIStageID ui_stage_;
    CameraID camera_id_;

    sig::signal<void (int)> signal_button_down_;
    sig::signal<void (int)> signal_button_up_;

    sig::signal<void ()> signal_left_down_;
    sig::signal<void ()> signal_left_up_;

    sig::signal<void ()> signal_right_down_;
    sig::signal<void ()> signal_right_up_;

};

}
#endif // VIRTUAL_GAMEPAD_H
