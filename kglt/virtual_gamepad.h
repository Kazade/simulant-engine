#ifndef VIRTUAL_GAMEPAD_H
#define VIRTUAL_GAMEPAD_H

#include <unordered_map>
#include <kazsignal/kazsignal.h>
#include "generic/managed.h"
#include "types.h"

#include "input_controller.h"

namespace kglt {

struct Dimensions {
    float top;
    float left;
    float width;
    float height;
};

class VirtualGamepad : public Managed<VirtualGamepad> {
public:
    VirtualGamepad(WindowBase& window, VirtualDPadDirections directions, int button_count);

    bool init();
    void cleanup();

    void flip();

    sig::signal<void (int)>& signal_button_down() { return signal_button_down_; }
    sig::signal<void (int)>& signal_button_up() { return signal_button_up_; }

    sig::signal<void (HatPosition)>& signal_hat_changed() { return signal_hat_changed_; }
    sig::signal<void (JoypadAxis, int)>& signal_axis_changed() { return signal_axis_changed_; }

    Dimensions button_dimensions(int button);

    void _disable_rendering();
private:
    WindowBase& window_;
    VirtualDPadDirections directions_ = VIRTUAL_DPAD_DIRECTIONS_TWO;
    int button_count_ = 0;

    std::unordered_map<int, std::set<int>> button_touches_;
    std::unordered_map<std::string, std::set<int>> dpad_button_touches_;

    UIStageID ui_stage_;
    CameraID camera_id_;
    PipelineID pipeline_id_;

    sig::signal<void (int)> signal_button_down_;
    sig::signal<void (int)> signal_button_up_;
    sig::signal<void (HatPosition)> signal_hat_changed_;
    sig::signal<void (JoypadAxis, int)> signal_axis_changed_;
};

}
#endif // VIRTUAL_GAMEPAD_H
