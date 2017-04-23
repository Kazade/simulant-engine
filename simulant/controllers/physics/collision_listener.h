#pragma once

#include <unordered_set>
#include "body.h"

namespace smlt {
namespace controllers {

class CollisionListener {
public:
    virtual ~CollisionListener() {
        auto watching = watching_;
        for(auto body: watching) {
            body->unregister_collision_listener(this);
        }
    }

private:
    virtual void on_collision_enter(const Collision& collision) {}
    virtual void on_collision_stay() {}
    virtual void on_collision_exit(const Collision& collision) {}

    virtual void on_trigger_enter() {}
    virtual void on_trigger_stay() {}
    virtual void on_trigger_exit() {}

    friend class impl::Body;

    std::unordered_set<impl::Body*> watching_;
};

}
}
