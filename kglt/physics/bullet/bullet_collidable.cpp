#include <mutex>

#include "../../object.h"
#include "../../kazbase/list_utils.h"
#include "bullet_collidable.h"
#include "bullet_engine.h"
#include "bullet_body.h"

namespace kglt {
namespace physics {

bool BulletCollidable::init() {

    return true;
}

void BulletCollidable::cleanup() {
    shapes_.clear();
}

void BulletCollidable::attach_to_responsive_body(ResponsiveBody& responsive) {
    BulletBody& body = dynamic_cast<BulletBody&>(responsive);

    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    auto child_list = body.compound_shape_->getChildList();
    std::vector<btCollisionShape*> shape_list;
    for(int32_t i = 0; i < body.compound_shape_->getNumChildShapes(); ++i) {
        shape_list.push_back((child_list + i)->m_childShape);
    }

    for(auto shape: shapes_) {
        if(container::contains(shape_list, shape.second.get())) {
            continue;
        }

        body.compound_shape_->addChildShape(btTransform(), shape.second.get());
    }
}

ShapeID BulletCollidable::add_plane(float a, float b, float c, float d) {
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    std::shared_ptr<btCollisionShape> new_geom = std::make_shared<btStaticPlaneShape>(btVector3(a, b, c), d);
    new_geom->setUserPointer(this);

    ShapeID new_id = get_next_shape_id();
    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }

    return new_id;
}

ShapeID BulletCollidable::add_sphere(float radius) {
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    std::shared_ptr<btCollisionShape> new_geom = std::make_shared<btSphereShape>(radius);
    new_geom->setUserPointer(this);

    ShapeID new_id = get_next_shape_id();
    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }
    return new_id;
}

ShapeID BulletCollidable::add_box(float width, float height, float depth) {
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    std::shared_ptr<btCollisionShape> new_geom = std::make_shared<btBoxShape>(btVector3(width / 2.0, height / 2.0, depth / 2.0));
    new_geom->setUserPointer(this);

    ShapeID new_id = get_next_shape_id();
    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }
    return new_id;
}

ShapeID BulletCollidable::add_capsule(float radius, float length) {
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    std::shared_ptr<btCollisionShape> new_geom = std::make_shared<btCapsuleShape>(radius, length);
    new_geom->setUserPointer(this);

    ShapeID new_id = get_next_shape_id();
    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }
    return new_id;
}

void BulletCollidable::set_relative_position(ShapeID shape, const Vec3& pos) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void BulletCollidable::set_absolute_position(ShapeID shape, const Vec3& pos) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void BulletCollidable::set_relative_rotation(ShapeID shape, const Quaternion& quat) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void BulletCollidable::set_absolute_rotation(ShapeID shape, const Quaternion& quat) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void BulletCollidable::set_bounciness(float value) {
    bounciness_ = value;
}

void BulletCollidable::set_friction(int32_t friction) {
    friction_ = friction;
}

float BulletCollidable::bounciness() const {
    return bounciness_;
}

int32_t BulletCollidable::friction() const {
    return friction_;
}

}
}
