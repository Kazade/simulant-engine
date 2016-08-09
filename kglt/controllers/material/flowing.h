#ifndef FLOWING_H
#define FLOWING_H

#include "../../material.h"
#include "../controller.h"
#include "../../generic/managed.h"

namespace kglt {
namespace controllers {
namespace material {

class Flowing :
    public MaterialController,
    public Managed<Flowing> {
public:
    Flowing(Controllable* material):
        MaterialController("flowing_material", dynamic_cast<Material*>(material)) {

    }

private:
    void do_update(double dt) override;
    double time_ = 0.0f;
};

}
}
}

#endif // FLOWING_H
