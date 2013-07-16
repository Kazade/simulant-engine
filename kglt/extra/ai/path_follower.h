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
    PathFollower(ActorHolder* parent);
    void follow(Path path);
    void enable_debug(bool value=true);

    void _update(double dt);

private:
    ActorHolder* actor_;
    Path path_;

    kglt::MeshID debug_mesh_;
    kglt::ActorID debug_actor_;
};

}
}

#endif // PATH_FOLLOWER_H
