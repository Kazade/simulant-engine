#ifndef COLLIDABLE_BODY_H
#define COLLIDABLE_BODY_H

#include <sigc++/sigc++.h>

#include "../generic/unique_id.h"
#include "../types.h"

namespace kglt {

class Object;
class ResponsiveBody;
class PhysicsEngine;

class Collidable {
public:
    Collidable(Object* owner, PhysicsEngine* engine);
    virtual ~Collidable() {}

    Object* owner() { return owner_; }
    PhysicsEngine* engine() { return engine_; }

    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual ShapeID add_sphere(float radius) = 0;
    virtual ShapeID add_box(float width, float height, float depth) = 0;
    virtual ShapeID add_capsule(float radius, float length) = 0;
    virtual ShapeID add_plane(float a, float b, float c, float d) = 0;

    virtual void set_relative_position(ShapeID shape, const Vec3& pos) = 0;
    virtual void set_absolute_position(ShapeID shape, const Vec3& pos) = 0;

    virtual void set_relative_rotation(ShapeID shape, const Quaternion& quat) = 0;
    virtual void set_absolute_rotation(ShapeID shape, const Quaternion& quat) = 0;

    virtual void attach_to_responsive_body(ResponsiveBody& body) = 0;

    sigc::signal<void, Collidable&> signal_collided() { return signal_collided_; }

    virtual void set_bounciness(float value) = 0;
    virtual float bounciness() const = 0;

    virtual void set_friction(int32_t friction) = 0;
    virtual int32_t friction() const = 0;

    bool has_infinite_friction() const { return friction() == -1; }

private:
    friend class PhysicsEngine;

    PhysicsEngine* engine_;
    Object* owner_;

protected:
    sigc::signal<void, Collidable&> signal_collided_;
};

}

#endif // COLLIDABLE_BODY_H
