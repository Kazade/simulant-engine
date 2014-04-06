#include "../object.h"
#include "../stage.h"
#include "../scene.h"

#include "responsive_body.h"

namespace kglt {

ResponsiveBody::ResponsiveBody(Object* owner):
    owner_(owner),
    engine_(&owner->stage()->scene().physics()){

    assert(engine_);
}

bool ResponsiveBody::init() {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    return do_init();
}

void ResponsiveBody::cleanup() {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_cleanup();
}

void ResponsiveBody::set_position(const kglt::Vec3& position) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_position(position);
}

kglt::Vec3 ResponsiveBody::position() const {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    return do_position();
}

void ResponsiveBody::set_rotation(const kglt::Quaternion& rotation) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_rotation(rotation);
}

kglt::Quaternion ResponsiveBody::rotation() const {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    return do_rotation();
}


void ResponsiveBody::set_mass_sphere(float total_mass, float radius) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_mass_sphere(total_mass, radius);
}

void ResponsiveBody::set_mass_box(float total_mass, float width, float height, float depth) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_mass_box(total_mass, width, height, depth);
}

void ResponsiveBody::apply_linear_impulse_global(const kglt::Vec3& impulse) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_linear_impulse_global(impulse);
}

void ResponsiveBody::apply_angular_impulse_global(const kglt::Vec3& impulse) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_angular_impulse_global(impulse);
}

void ResponsiveBody::apply_angular_impulse_local(const Vec3 &impulse) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_angular_impulse_local(impulse);
}

void ResponsiveBody::apply_linear_impulse_local(const Vec3 &impulse) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_linear_impulse_local(impulse);
}

void ResponsiveBody::apply_linear_force_global(const kglt::Vec3& force) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_linear_force_global(force);
}

void ResponsiveBody::apply_linear_force_local(const kglt::Vec3& force) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_linear_force_local(force);
}

void ResponsiveBody::apply_angular_force_global(const kglt::Vec3& force) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_angular_force_global(force);
}

void ResponsiveBody::apply_angular_force_local(const kglt::Vec3& force) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_apply_angular_force_local(force);
}

void ResponsiveBody::set_angular_damping(const float amount) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_angular_damping(amount);
}

void ResponsiveBody::set_linear_damping(const float amount) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_linear_damping(amount);
}


void ResponsiveBody::set_angular_velocity(const kglt::Vec3& velocity) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_angular_velocity(velocity);
}

kglt::Vec3 ResponsiveBody::angular_velocity() const {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    return do_angular_velocity();
}

void ResponsiveBody::set_linear_velocity(const kglt::Vec3& velocity) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_set_linear_velocity(velocity);
}

kglt::Vec3 ResponsiveBody::linear_velocity() const {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    return do_linear_velocity();
}

ConstraintID ResponsiveBody::create_fixed_constraint(ResponsiveBody& other) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    return do_create_fixed_constraint(other);
}

ConstraintID ResponsiveBody::create_pivot_constraint(ResponsiveBody& other, const kglt::Vec3& pivot) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    return do_create_pivot_constraint(other, pivot);
}

void ResponsiveBody::destroy_constraint(ConstraintID c) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_destroy_constraint(c);
}

void ResponsiveBody::enable_constraint(ConstraintID c) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_enable_constraint(c);
}

void ResponsiveBody::disable_constraint(ConstraintID c) {
    std::lock_guard<std::recursive_mutex> lock(engine_->mutex());
    do_disable_constraint(c);
}

}
