#ifndef PATH_FOLLOWER_H
#define PATH_FOLLOWER_H

#include "../../generic/managed.h"
#include "../../base.h"
#include "path.h"

namespace kglt {
namespace extra {

class Path;

class PathFollower:
    public Managed<PathFollower> {
public:
    PathFollower(ActorHolder* parent, float max_speed, float max_force);
    void follow(Path path, bool loop=false);
    void enable_debug(bool value=true);

    kglt::Vec3 force_to_apply(const kglt::Vec3& velocity) const;

private:
    kglt::Vec3 seek(const kglt::Vec3& target, const Vec3 &velocity) const;

    ActorHolder* actor_;
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
