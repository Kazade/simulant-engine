#ifndef PATH_FOLLOWER_H
#define PATH_FOLLOWER_H

#include "../../kazbase/exceptions.h"
#include "../../generic/managed.h"
#include "../../base.h"
#include "path.h"


namespace kglt {
namespace extra {

class Path;

class PathFollower:
    public Managed<PathFollower> {

public:
    PathFollower(MoveableActorHolder* parent, float max_speed, float max_force);
    void follow(Path path);
    void enable_debug(bool value=true);

    kglt::Vec3 force_to_apply(const kglt::Vec3& velocity);

    //Pass throughs to the actor
    kglt::Vec3 position() const { return actor_->position(); }
    kglt::Vec3 setPosition(kglt::Vec3 p) { actor_->set_position(p); return position();  }
    kglt::Vec3 velocity() const { return actor_->velocity(); }
    float maxForce() const { return actor_->max_force(); }
    float speed() const { return velocity().length(); }
    float maxSpeed() const { return actor_->max_speed(); }
    float radius() const { return actor_->radius(); }
    float mass() const { return actor_->mass(); }
private:
    kglt::Vec3 seek(const kglt::Vec3& target, const Vec3 &velocity) const;

    MoveableActorHolder* actor_;
    float max_speed_;
    float max_force_;

    Path path_;
    bool loop_ = false;

    kglt::MeshID debug_mesh_;
    kglt::ActorID debug_actor_;
    kglt::SubMeshIndex normal_points_mesh_;

    mutable std::vector<kglt::Vec3> normal_points_;
    void update_debug_mesh() const;
};

}
}

#endif // PATH_FOLLOWER_H
