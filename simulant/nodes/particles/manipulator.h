#pragma once

#include <list>
#include <string>
#include "particle.h"


namespace smlt {
namespace particles {

class Manipulator {
public:
    Manipulator(const std::string& name):
        name_(name) {}

    virtual void set_property(const std::string& name, int32_t value) {}
    virtual void set_property(const std::string& name, float value) {}

    void manipulate(std::list<Particle>& particles, float dt) {
        do_manipulate(particles, dt);
    }

private:
    std::string name_;
    virtual void do_manipulate(std::list<Particle>& particles, float dt) = 0;
};



}
}
