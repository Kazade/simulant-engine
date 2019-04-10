#pragma once

#include <list>
#include <string>
#include <functional>
#include "particle.h"
#include "manipulators/curves.h"

namespace smlt {
namespace particles {

class Manipulator {
public:
    Manipulator(const std::string& name):
        name_(name) {}

    void manipulate(std::vector<Particle>& particles, float dt) {
        do_manipulate(particles, dt);
    }

    void set_linear_curve(float rate) {
        using namespace std::placeholders;
        curve_ = std::bind(&linear_curve, _1, _2, _3, rate);
    }

    void set_bell_curve(float peak, float deviation) {
        using namespace std::placeholders;
        curve_ = std::bind(&bell_curve, _1, _2, _3, peak, deviation);
    }

private:
    std::string name_;
    virtual void do_manipulate(std::vector<Particle>& particles, float dt) = 0;

protected:
    std::function<float (float, float, float)> curve_;
};

typedef std::shared_ptr<Manipulator> ManipulatorPtr;

}
}
