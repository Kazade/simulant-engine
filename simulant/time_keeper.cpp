#ifdef __DREAMCAST__
#include <kos.h>
#endif

#include <iostream>
#include "time_keeper.h"

namespace smlt {

TimeKeeper::TimeKeeper(const float fixed_step):
    fixed_step_(fixed_step) {

}

uint64_t TimeKeeper::now_in_us() {
#ifdef __DREAMCAST__
    return timer_us_gettime64();
#elif defined(__PSP__)
    return sceKernelGetSystemTimeWide();
#else
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
#endif
}

bool TimeKeeper::init() {
    last_update_ = now_in_us();
    return true;
}

void TimeKeeper::clean_up() {

}

void TimeKeeper::update() {
    const float DELTATIME_MAX = 0.25f;
    const float ACCUMULATOR_MAX = 0.25f;

    auto now = now_in_us();
    auto diff = now - last_update_;
    last_update_ = now;

    delta_time_ = float(diff) * 0.000001f;

    delta_time_ = std::min(DELTATIME_MAX, delta_time_);

    accumulator_ += delta_time_;
    accumulator_ = std::min(ACCUMULATOR_MAX, accumulator_);

    total_time_ += delta_time_;
}

float TimeKeeper::fixed_step_remainder() const {

    // Don't return anything if we have fixed steps remaining
    if(accumulator_ >= fixed_step_) {
        return 0.0f;
    }

    return accumulator_;
}

bool TimeKeeper::use_fixed_step() {
    bool can_update = accumulator_ >= fixed_step_;

    if(can_update) {
        accumulator_ -= fixed_step_;
    }

    return can_update;
}

}
