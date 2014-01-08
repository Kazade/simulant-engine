#ifndef BOID_H
#define BOID_H

#include <kazbase/exceptions.h>

#include "../../generic/managed.h"
#include "../../base.h"
#include "path.h"


namespace kglt {
namespace extra {

class Path;

class Boid:
    public Managed<Boid> {

public:
    Boid(MoveableActorHolder* parent, float max_speed, float max_force);
    void follow(Path path);
    void enable_debug(bool value=true);

    kglt::Vec3 steer_to_path();
    kglt::Vec3 seek(const Vec3& target, float slowing_radius=2.0) const;
    kglt::Vec3 flee(const Vec3& target) const;
    kglt::Vec3 pursue(const Vec3& target, const Vec3& target_velocity, const float target_max_speed) const;
    kglt::Vec3 evade(const Vec3& target, const Vec3& target_velocity, const float target_max_speed) const;

    kglt::Vec3 separate(const std::vector<Boid::ptr> others);

private:
    MoveableActorHolder* actor_;

    Path path_;
    int32_t current_node_ = 0;

    bool loop_ = false;

    kglt::MeshID debug_mesh_;
    kglt::ActorID debug_actor_;
    kglt::SubMeshIndex normal_points_mesh_;

    mutable std::vector<kglt::Vec3> normal_points_;
    void update_debug_mesh() const;
};

float map(float value, float min, float max, float new_min, float new_max);

}
}

#endif // PATH_FOLLOWER_H
