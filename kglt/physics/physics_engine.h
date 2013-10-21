#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include <memory>

#include "../types.h"
#include "../kazbase/base/taggable_object.h"
#include "../generic/protected_ptr.h"

#include "../kazbase/signals3/signals3.hpp"

namespace kglt {

class ResponsiveBody;
class Collidable;

class Object;

class PhysicsEngine {
public:
    typedef sig::signal<void (Collidable&, Collidable&)> CombinedCollisionSignal;

    bool init();
    void cleanup();

    void step(double dt);

    //Factory function
    std::shared_ptr<ResponsiveBody> new_responsive_body(Object* owner);
    std::shared_ptr<Collidable> new_collidable(Object* owner);

    ShapeID create_plane(float a, float b, float c, float d);
    void set_gravity(const kglt::Vec3& gravity);

    CombinedCollisionSignal& collision_signal_for(base::Tag obj_tag1, base::Tag obj_tag2);

    std::recursive_mutex& mutex() { return mutex_; }

private:
    std::recursive_mutex mutex_;

protected:
    void fire_collision_signals_for(Collidable& lhs, Collidable& rhs);

private:
    typedef std::map< std::pair<base::Tag, base::Tag>, CombinedCollisionSignal> SignalMap;
    SignalMap collision_signals_;

    virtual bool do_init() = 0;
    virtual void do_cleanup() = 0;
    virtual void do_step(double dt) = 0;
    virtual std::shared_ptr<ResponsiveBody> do_new_responsive_body(Object* owner) = 0;
    virtual std::shared_ptr<Collidable> do_new_collidable(Object* owner) = 0;

    virtual ShapeID do_create_plane(float a, float b, float c, float d) = 0;
    virtual void do_set_gravity(const kglt::Vec3& gravity) = 0;
};

ShapeID get_next_shape_id();

}

#endif // PHYSICS_ENGINE_H
