#pragma once

#include "../manipulator.h"
#include "curves.h"

namespace smlt {
namespace particles {

class SizeManipulator : public Manipulator {
public:
    SizeManipulator(ParticleSystem* system):
        Manipulator(system, "scalar") {}


private:
    void set_bell_curve(float peak, float deviation) override {
        is_bell_curve_ = true;
        is_linear_curve_ = false;
        peak_ = peak;
        deviation_ = deviation;

        Manipulator::set_bell_curve(peak, deviation);
    }

    void set_linear_curve(float rate) override {
        is_linear_curve_ = true;
        is_bell_curve_ = false;

        rate_ = rate;

        Manipulator::set_linear_curve(rate);
    }

    void do_manipulate(std::vector<Particle>& particles, float dt) override;

    bool is_bell_curve_ = false;
    bool is_linear_curve_ = false;

    float peak_ = 0.0f;
    float rate_ = 0.0f;
    float deviation_ = 0.0f;
};


}
}
