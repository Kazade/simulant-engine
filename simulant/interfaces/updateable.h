#pragma once

#include "../macros.h"

namespace smlt {

/**
 * @brief The Updateable class
 *
 * Any object that can be updated using a deltatime value
 *
 */
class Updateable {
public:
    virtual ~Updateable() {}

    /*
     * Non-Virtual Interface. Simulant calls these underscore prefixed
     * functions, and subclasses implement the more nicely named ones
     */
    virtual void _update_thunk(float dt) {
        update(dt);
    }

    virtual void _late_update_thunk(float dt) {
        late_update(dt);
    }

    virtual void _fixed_update_thunk(float step) {
        fixed_update(step);
    }

private:
    virtual void update(float dt) {
        _S_UNUSED(dt);
    }
    virtual void late_update(float dt) {
        _S_UNUSED(dt);
    }
    virtual void fixed_update(float step) {
        _S_UNUSED(step);
    }
};

}
