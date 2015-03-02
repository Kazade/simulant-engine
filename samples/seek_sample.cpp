#include "kglt/kglt.h"
#include "kglt/base.h"
#include "kglt/shortcuts.h"
#include "kglt/extra.h"
#include "kglt/extra/ai/boid.h"

using namespace kglt;
using namespace kglt::extra;

class Dot:
    public kglt::MoveableActorHolder,
    public Managed<Dot> {

public:
    Dot(kglt::WindowBase& window, kglt::StageID stage):
        kglt::MoveableActorHolder(window),
        stage_(stage) {

        //Pass a reference to this to the PathFollower
        follower_.reset(new Boid(this, 1, 1));
    }

    bool init() {
        actor_ = stage()->geom_factory().new_cube(1);
        follower_->enable_debug();

        set_position(kglt::Vec3(-10, 0, -50));
        set_velocity(kglt::Vec3(10, 0, 0));
        set_max_speed(10.0);

        return true;
    }

    void update(double dt) {
        set_velocity(follower_->seek(target));

        actor()->move_to(position() + (velocity() * dt));

        //Keep moving the target when we reach it
        if((position() - target).length() < 0.01) {
            target.x = -target.x;
        }
    }

    kglt::ActorID actor_id() const { return actor_; }
    kglt::StageID stage_id() const { return stage_; }

private:
    Boid::ptr follower_;

    kglt::StageID stage_;
    kglt::ActorID actor_;

    kglt::Vec3 target = kglt::Vec3(10, 0, -50);
};

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(WindowBase& window):
        kglt::Screen<GameScreen>(window) {}

    void do_load() {
        prepare_basic_scene(stage_id_, camera_id_);

        dot_ = Dot::create(window, stage_id_);

        window->camera(camera_id_)->set_perspective_projection(
            45.0,
            float(window->width()) / float(window->height()),
            1.0,
            1000.0
        );
    }

    void do_step(double dt) {
        dot_->update(dt);
    }

private:
    StageID stage_id_;
    CameraID camera_id_;
    Dot::ptr dot_;
};

class SeekSample: public kglt::Application {
public:
    SeekSample():
        Application("KGLT Seek Behaviour") {

        window->set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {
        register_screen("/", screen_factory<GameScreen>());
        return true;
    }
};


int main(int argc, char* argv[]) {
    SeekSample app;
    return app.run();
}


