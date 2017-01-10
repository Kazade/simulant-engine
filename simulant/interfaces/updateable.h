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
    virtual void update(double step) = 0;
    virtual void pre_update(double dt) {}
    virtual void post_update(double dt) {}

    virtual void pre_fixed_update(double step) {}
    virtual void fixed_update(double step) {}
    virtual void post_fixed_update(double step) {}
};

}
