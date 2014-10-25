#ifndef VIRTUAL_GAMEPAD_H
#define VIRTUAL_GAMEPAD_H

#include "generic/managed.h"
#include "types.h"

namespace kglt {

class VirtualGamepad : public Managed<VirtualGamepad> {
public:
    VirtualGamepad(WindowBase& window, VirtualDPadDirections directions, int button_count);

    bool init();
    void cleanup();

    void flip();

private:
    WindowBase& window_;
    VirtualDPadDirections directions_ = VIRTUAL_DPAD_DIRECTIONS_TWO;
    int button_count_ = 0;

    UIStageID ui_stage_;
    CameraID camera_id_;
};

}
#endif // VIRTUAL_GAMEPAD_H
