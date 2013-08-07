#include "types.h"
#include "physics_engine.h"
#include "collidable.h"

namespace kglt {

static uint32_t counter = 0;

ShapeID get_next_shape_id() {
    return ShapeID(++counter);
}

bool PhysicsEngine::init() {
    std::lock_guard<std::recursive_mutex> guard(this->mutex());
    return do_init();
}

void PhysicsEngine::cleanup() {
    std::lock_guard<std::recursive_mutex> guard(this->mutex());
    do_cleanup();
}

void PhysicsEngine::step(double dt) {
    std::lock_guard<std::recursive_mutex> guard(this->mutex());
    do_step(dt);
}

std::shared_ptr<ResponsiveBody> PhysicsEngine::new_responsive_body(Object* owner) {
    std::lock_guard<std::recursive_mutex> guard(this->mutex());
    return do_new_responsive_body(owner);
}

std::shared_ptr<Collidable> PhysicsEngine::new_collidable(Object* owner) {
    std::lock_guard<std::recursive_mutex> guard(this->mutex());
    return do_new_collidable(owner);
}

ShapeID PhysicsEngine::create_plane(float a, float b, float c, float d) {
    std::lock_guard<std::recursive_mutex> guard(this->mutex());
    return do_create_plane(a, b, c, d);
}

void PhysicsEngine::set_gravity(const kglt::Vec3& gravity) {
    std::lock_guard<std::recursive_mutex> guard(this->mutex());
    do_set_gravity(gravity);
}

void PhysicsEngine::fire_collision_signals_for(Collidable& lhs, Collidable& rhs) {
    base::TagList tags1 = lhs.get_tags();
    base::TagList tags2 = rhs.get_tags();

    std::set<std::pair<base::Tag, base::Tag> > combos;
    for(base::Tag t1: tags1) {
        for(base::Tag t2: tags2) {
            if(t2 < t1) {
                std::swap(t1, t2);
            }

            combos.insert(std::make_pair(t1, t2));
        }
    }

    //Fire any relevant signals
    for(auto key: combos) {
        auto it = collision_signals_.find(key);
        if(it != collision_signals_.end()) {
            (*it).second(lhs, rhs);
        }
    }
}

PhysicsEngine::CombinedCollisionSignal PhysicsEngine::collision_signal_for(base::Tag obj_tag1, base::Tag obj_tag2) {
    //Make sure the tags are in order
    if(obj_tag2 < obj_tag1) {
        std::swap(obj_tag1, obj_tag2);
    }

    auto key = std::make_pair(obj_tag1, obj_tag2);

    return collision_signals_[key];
}

}
