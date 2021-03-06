#pragma once

#ifdef __DREAMCAST__
    #include <cstdint>
#else
    #include <chrono>
#endif

#include "generic/managed.h"

namespace smlt {

class TimeKeeper:
    public RefCounted<TimeKeeper> {

public:
    TimeKeeper(const float fixed_step);

    bool init() override;
    void clean_up() override;

    void update();

    static uint64_t now_in_us();

    float delta_time() const { return delta_time_; }
    float fixed_step() const { return fixed_step_; }
    float fixed_step_remainder() const;
    float total_elapsed_seconds() const { return total_time_; }

    bool use_fixed_step();

    void restart() {
        total_time_ = delta_time_ = accumulator_ = 0.0f;
    }

private:
    uint64_t last_update_;

    float accumulator_ = 0.0f;
    float total_time_ = 0.0f;
    float delta_time_ = 0.0f;
    float fixed_step_ = 0.0f;
};

}
