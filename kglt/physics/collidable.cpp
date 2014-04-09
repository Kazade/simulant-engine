#include "../stage.h"
#include "collidable.h"
#include "../object.h"

namespace kglt {

Collidable::Collidable(Object* owner, PhysicsEnginePtr engine):
    engine_(engine),
    owner_(owner) {

    assert(owner_ || engine_);

    if(owner_) {
        owner->signal_made_responsive().connect([=]() {
            attach_to_responsive_body(owner->body());
        });

        if(!engine_) {
            engine_ = owner_->stage()->window().scene().physics();
        }
    }

    assert(engine_);
}

}
