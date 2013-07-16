#include "kglt/kglt.h"
#include "kglt/base.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"
#include "kglt/extra/ai/path_follower.h"

using namespace kglt::extra;

class Car:
    public kglt::ActorHolder {

public:
    Car(Scene& scene):
        kglt::ActorHolder(scene) {

        follower_.reset(new PathFollower(this));
    }

private:
    PathFollower::ptr follower_;
};


class PathFollowing: public kglt::App {
public:
    PathFollowing():
        App("KGLT Sprite Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {

        return true;
    }

    void do_step(double dt) {}
    void do_cleanup() {}

    Sprite::ptr sprite_;


};


int main(int argc, char* argv[]) {
    PathFollowing app;
    return app.run();
}

