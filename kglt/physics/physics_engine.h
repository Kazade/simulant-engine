#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include <sigc++/sigc++.h>

#include "../types.h"
#include "../kazbase/base/taggable_object.h"

namespace kglt {

class ResponsiveBody;
class Collidable;

class Object;

class PhysicsEngine {
public:
    typedef sigc::signal<void, Collidable&, Collidable&> CombinedCollisionSignal;

    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual void step(double dt) = 0;

    //Factory function
    virtual std::shared_ptr<ResponsiveBody> new_responsive_body(Object* owner) = 0;
    virtual std::shared_ptr<Collidable> new_collidable(Object* owner) = 0;

    virtual ShapeID create_plane(float a, float b, float c, float d) = 0;
    virtual void set_gravity(const kglt::Vec3& gravity) = 0;

    CombinedCollisionSignal collision_signal_for(base::Tag obj_tag1, base::Tag obj_tag2);

protected:
    void fire_collision_signals_for(Collidable& lhs, Collidable& rhs);

private:
    typedef std::map< std::pair<base::Tag, base::Tag>, CombinedCollisionSignal> SignalMap;
    SignalMap collision_signals_;
};

ShapeID get_next_shape_id();

}

#endif // PHYSICS_ENGINE_H
