
#include "time_keeper.h"

namespace smlt {

TimeKeeper::TimeKeeper(const float fixed_step):
    fixed_step_(fixed_step) {

}

bool TimeKeeper::init() {
    ktiGenTimers(1, &fixed_timer_);
    ktiBindTimer(fixed_timer_);
    ktiStartFixedStepTimer(1.0f / fixed_step_);

    ktiGenTimers(1, &variable_timer_);
    ktiBindTimer(variable_timer_);
    ktiStartGameTimer();

    return true;
}

void TimeKeeper::cleanup() {
    ktiDeleteTimers(1, &fixed_timer_);
    ktiDeleteTimers(1, &variable_timer_);
}

void TimeKeeper::update() {
    ktiBindTimer(fixed_timer_);
    ktiUpdateFrameTime();

    ktiBindTimer(variable_timer_);
    ktiUpdateFrameTime();

    // Store the frame time, and the total elapsed time
    delta_time_ = ktiGetDeltaTime();
    total_time_ += delta_time_;
}

float TimeKeeper::fixed_step_remainder() const {
    ktiBindTimer(fixed_timer_);
    return ktiGetAccumulatorValue();
}

bool TimeKeeper::use_fixed_step() {
    ktiBindTimer(fixed_timer_);
    return ktiTimerCanUpdate();
}

}
