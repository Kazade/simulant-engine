#ifndef WARP_H
#define WARP_H

#include "../../material.h"
#include "../controller.h"
#include "../../generic/managed.h"

namespace kglt {
namespace controllers {
namespace material {

class Warp:
    public MaterialController,
    public Managed<Warp> {

public:
    Warp(Controllable* material):
        MaterialController("warp_material", dynamic_cast<Material*>(material)) {

    }

private:
    void do_update(double dt) override;
    double time_ = 0.0f;
};

}
}
}

#endif // WARP_H
