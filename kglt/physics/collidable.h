#ifndef COLLIDABLE_BODY_H
#define COLLIDABLE_BODY_H

#include "../generic/unique_id.h"
#include "../types.h"

namespace kglt {

class Object;
class ResponsiveBody;

class Collidable {
public:
    Collidable(Object* owner);
    virtual ~Collidable() {}

    Object* owner() { return owner_; }

    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual ShapeID add_sphere(float radius) = 0;
    virtual ShapeID add_box(float width, float height, float depth) = 0;
    virtual ShapeID add_capsule(float radius, float length) = 0;

    virtual void set_relative_position(ShapeID shape, const Vec3& pos) = 0;
    virtual void set_absolute_position(ShapeID shape, const Vec3& pos) = 0;

    virtual void set_relative_rotation(ShapeID shape, const Quaternion& quat) = 0;
    virtual void set_absolute_rotation(ShapeID shape, const Quaternion& quat) = 0;

    virtual void attach_to_responsive_body(ResponsiveBody& body) = 0;
private:
    Object* owner_;

};

}

#endif // COLLIDABLE_BODY_H
