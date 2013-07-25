#include "kglt/kglt.h"
#include "kglt/base.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"
#include "kglt/extra/ai/path_follower.h"

using namespace kglt::extra;

class Car:
    public kglt::MoveableActorHolder,
    public Managed<Car> {

public:
    Car(kglt::Scene& scene, kglt::StageID stage):
        kglt::MoveableActorHolder(scene),
        stage_(stage) {

        //Pass a reference to this to the PathFollower
        follower_.reset(new PathFollower(this, 1, 1));
    }

    bool init() {
        actor_ = stage()->geom_factory().new_cube(1);

        Path p(2.0);

        p.add_point(kglt::Vec3(-10, -10, -50));
        p.add_point(kglt::Vec3( 10, -10, -50));
        p.add_point(kglt::Vec3( 10,  10, -50));
        p.add_point(kglt::Vec3(-10,  10, -50));
        p.add_point(kglt::Vec3(-9,  7, -50));
        p.add_point(kglt::Vec3(-12,  5, -50));
        p.add_point(kglt::Vec3(-5,  3, -50));
        p.add_point(kglt::Vec3(-10, -10, -50));

        follower_->follow(p);

        set_velocity(kglt::Vec3(5, 0, 0));
        set_min_speed(5.0);
        set_max_speed(10.0);

        follower_->enable_debug();

        return true;
    }

    void update(double dt) {        
        set_velocity(follower_->steer_to_path());
        actor()->move_to(position() + (velocity() * dt));
    }

    kglt::ActorID actor_id() const { return actor_; }
    kglt::StageID stage_id() const { return stage_; }

private:
    PathFollower::ptr follower_;

    kglt::Vec3 velocity_ = kglt::Vec3(1, 0, 0);
    kglt::Vec3 acceleration_;

    kglt::StageID stage_;
    kglt::ActorID actor_;
};


class PathFollowing: public kglt::App {
public:
    PathFollowing():
        App("KGLT Sprite Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {
        car_ = Car::create(scene(), stage().id());

        scene().camera().set_perspective_projection(
            45.0,
            float(window().width()) / float(window().height()),
            1.0,
            1000.0
        );
        return true;
    }

    void do_step(double dt) {
        if(initialized()) {
            car_->update(dt);
        }
    }

    void do_cleanup() {}

    Car::ptr car_;
};


int main(int argc, char* argv[]) {
    PathFollowing app;
    return app.run();
}

