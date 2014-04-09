#ifndef COLLIDABLE_BODY_H
#define COLLIDABLE_BODY_H

#include "../generic/unique_id.h"
#include <kazbase/base/taggable_object.h>
#include "../types.h"

namespace kglt {

class Object;
class ResponsiveBody;
class PhysicsEngine;

class Collidable : public base::TaggableObject {
public:
    Collidable(Object* owner, PhysicsEnginePtr engine);
    virtual ~Collidable() {}

    Object* owner() { return owner_; }
    PhysicsEnginePtr engine() { return engine_; }

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

    sig::signal<void (Collidable&)>& signal_collided() { return signal_collided_; }

    virtual void set_bounciness(float value) = 0;
    virtual float bounciness() const = 0;

    virtual void set_friction(int32_t friction) = 0;
    virtual int32_t friction() const = 0;

    bool has_infinite_friction() const { return friction() == -1; }

    //If a collidable is a ghost, it triggers collision signals but objects
    //pass through it
    void set_is_ghost(bool val=true) { is_ghost_ = val; }
    bool is_ghost() const { return is_ghost_; }

    void set_response_test_callback(std::function<bool (Collidable&)> func) {
        should_respond_callback_ = func;
    }

private:
    friend class PhysicsEngine;

    PhysicsEnginePtr engine_;
    Object* owner_;
    bool is_ghost_ = false;

protected:
    sig::signal<void (Collidable&)> signal_collided_;

    std::function<bool (Collidable&)> should_respond_callback_;
};

}

#endif // COLLIDABLE_BODY_H
