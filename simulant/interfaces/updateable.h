#pragma once

#include "../macros.h"
#include "../signals/signal.h"

namespace smlt {

typedef sig::signal<void (float)> UpdatedSignal;
typedef sig::signal<void (float)> LateUpdatedSignal;
typedef sig::signal<void (float)> FixedUpdatedSignal;

/**
 * @brief The Updateable class
 *
 * Any object that can be updated using a deltatime value
 *
 */
class Updateable {
    DEFINE_SIGNAL(UpdatedSignal, signal_update);
    DEFINE_SIGNAL(LateUpdatedSignal, signal_late_update);
    DEFINE_SIGNAL(FixedUpdatedSignal, signal_fixed_update);

public:
    virtual ~Updateable() {}

    /*
     * Non-Virtual Interface. Simulant calls these underscore prefixed
     * functions, and subclasses implement the more nicely named ones
     */
    virtual void update(float dt) {
        on_update(dt);
        signal_update_(dt);
    }

    virtual void late_update(float dt) {
        on_late_update(dt);
        signal_late_update_(dt);
    }

    virtual void fixed_update(float step) {
        on_fixed_update(step);
        signal_fixed_update_(step);
    }

protected:
    virtual void on_update(float dt) {
        _S_UNUSED(dt);
    }
    virtual void on_late_update(float dt) {
        _S_UNUSED(dt);
    }
    virtual void on_fixed_update(float step) {
        _S_UNUSED(step);
    }
};

}
