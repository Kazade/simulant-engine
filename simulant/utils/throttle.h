#pragma once

namespace smlt {

class Throttle {
public:
    Throttle(uint16_t fps):
        wait_time_(1.0f / fps) {}

    /* Returns true if it's time for another update */
    bool update_and_test(float dt) {
        passed_ += dt;
        if(passed_ >= wait_time_) {
            passed_ -= wait_time_;
            return true;
        }
        return false;
    }

private:
    float passed_ = 0.0f;
    float wait_time_ = 0.0f;
};

}
