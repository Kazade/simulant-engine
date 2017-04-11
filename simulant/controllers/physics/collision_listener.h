#pragma once

namespace smlt {
namespace controllers {

class CollisionListener {
public:


private:
    virtual void on_collision_enter() {}
    virtual void on_collision_stay() {}
    virtual void on_collision_exit() {}

    virtual void on_trigger_enter() {}
    virtual void on_trigger_stay() {}
    virtual void on_trigger_exit() {}
};

}
}
