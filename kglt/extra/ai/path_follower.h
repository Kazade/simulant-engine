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
    void follow(Path path);
    void enable_debug(bool value=true);

    void _update(double dt);

    kglt::Vec3 force_to_apply(const kglt::Vec3& velocity) const;

private:
    void seek(const kglt::Vec3& target);

    kglt::Vec3 get_normal_point(const kglt::Vec3& p, const kglt::Vec3& a, const kglt::Vec3& b);
    bool point_on_line(const kglt::Vec3& p, const kglt::Vec3& a, const kglt::Vec3& b);

    kglt::Vec3 forward();

    ActorHolder* actor_;
    float max_speed_;
    float max_force_;

    Path path_;

    kglt::MeshID debug_mesh_;
    kglt::ActorID debug_actor_;

    kglt::Vec3 force_to_apply_;
};

}
}

#endif // PATH_FOLLOWER_H
