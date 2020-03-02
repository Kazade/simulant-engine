#ifdef _arch_dreamcast
#include <kos.h>
#endif

#include <iostream>
#include "time_keeper.h"

namespace smlt {

TimeKeeper::TimeKeeper(const float fixed_step):
    fixed_step_(fixed_step) {

}

uint64_t TimeKeeper::now_in_us() {
#ifdef _arch_dreamcast
    return timer_us_gettime64();
#else
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
#endif
}

bool TimeKeeper::init() {
#ifdef _arch_dreamcast
    last_update_ = timer_us_gettime64();
#else
    last_update_ = std::chrono::high_resolution_clock::now();
#endif
    return true;
}

void TimeKeeper::clean_up() {

}

void TimeKeeper::update() {
    const float DELTATIME_MAX = 0.25f;
    const float ACCUMULATOR_MAX = 0.25f;

#ifdef _arch_dreamcast
    auto now = timer_us_gettime64();
    auto diff = now - last_update_;
    last_update_ = now;

    delta_time_ = float(diff) * 0.000001f;
    delta_time_ = std::min(DELTATIME_MAX, delta_time_);

#else
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> seconds = now - last_update_;
    last_update_ = now;

    // Store the frame time, and the total elapsed time
    delta_time_ = seconds.count();
#endif

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
