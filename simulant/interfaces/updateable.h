#pragma once


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
    virtual void _update_thunk(double dt) {
        update(dt);
    }

    virtual void _late_update_thunk(double dt) {
        late_update(dt);
    }

    virtual void _fixed_update_thunk(double step) {
        fixed_update(step);
    }

private:
    virtual void update(double dt) {}
    virtual void late_update(double dt) {}
    virtual void fixed_update(double step) {}
};

}
