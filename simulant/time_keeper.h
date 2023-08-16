#pragma once

#ifdef __DREAMCAST__
    #include <cstdint>
#else
    #include <chrono>
#endif

#include "generic/managed.h"
#include "threads/atomic.h"

namespace smlt {

class TimeKeeper:
    public RefCounted<TimeKeeper> {

public:
    TimeKeeper(const float fixed_step);

    bool on_init() override;
    void on_clean_up() override;

    void update();

    static uint64_t now_in_us();

    float delta_time() const { return delta_time_ * time_scale_; }
    float fixed_step() const { return fixed_step_ * time_scale_; }
    float fixed_step_remainder() const;
    float total_elapsed_seconds() const { return total_time_; }

    float unscaled_delta_time() const { return delta_time_; }
    float unscaled_fixed_step() const { return fixed_step_; }

    float time_scale() const;

    /**
     * Scale deltatime and fixed step by a multiplier value. Setting this to 1.0 will
     * return deltatime to normal, setting to zero will mean that deltatime and fixed_step
     * will both return zero.
     *
     * update will continue to be called even if time_scale == 0.0f
     * fixed_update will stop being called, otherwise the accumulator would never
     * decrease and it would run infinitely.
    */
    void set_time_scale(float value);

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
    thread::Atomic<float> time_scale_;
};

}
