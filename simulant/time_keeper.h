#pragma once

#include <chrono>

#include "generic/managed.h"

namespace smlt {

class TimeKeeper:
    public Managed<TimeKeeper> {

public:
    TimeKeeper(const float fixed_step);

    bool init() override;
    void cleanup() override;

    void update();

    float delta_time() const { return delta_time_; }
    float fixed_step() const { return fixed_step_; }
    float fixed_step_remainder() const;
    float total_elapsed_seconds() const { return total_time_; }

    bool use_fixed_step();

private:
    std::chrono::high_resolution_clock::time_point last_update_;

    float accumulator_ = 0.0f;
    float total_time_ = 0.0f;
    float delta_time_ = 0.0f;
    float fixed_step_ = 0.0f;
};

}
