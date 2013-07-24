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

    kglt::Vec3 seek(const kglt::Vec3& target, const Vec3 &velocity, float slowing_radius=2.0) const;

private:
    MoveableActorHolder* actor_;

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
