#pragma once

#include "body.h"

namespace smlt {
namespace controllers {

class CollisionListener {
public:


private:
    virtual void on_collision_enter(const Collision& collision) {}
    virtual void on_collision_stay() {}
    virtual void on_collision_exit(const Collision& collision) {}

    virtual void on_trigger_enter() {}
    virtual void on_trigger_stay() {}
    virtual void on_trigger_exit() {}

    friend class impl::Body;
};

}
}
