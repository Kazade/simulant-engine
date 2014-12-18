#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/extra.h"

using namespace kglt::extra;
using namespace kglt;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(WindowBase& window):
        kglt::Screen<GameScreen>(window) {}

    void do_load() {
        window().enable_physics(DefaultPhysicsEngine::create());
        window().physics()->create_plane(0, 1, 0, -3.5);
        window().physics()->set_gravity(Vec3(0, -7.8, 0));

        texture_id_ = window().stage()->new_texture_from_file("sample_data/crate.png");
        mesh_ = window().stage()->new_mesh_as_cube(1.0);
        window().stage()->mesh(mesh_)->set_texture_on_material(0, texture_id_);

        window().stage()->set_ambient_light(kglt::Colour(0.3, 0.3, 0.3, 0.3));

        LightID lid = window().stage()->new_light();
        {
            auto light = window().stage()->light(lid);
            light->set_direction(-1, 0, 0);
            light->set_diffuse(kglt::Colour(0.1, 0.1, 0.1, 0.1));
            light->set_specular(kglt::Colour(0, 0, 0, 0));
        }
    }

    void do_step(double dt) {
        time_since_last_spawn_ += dt;

        //Spawn a new cube every second
        if(time_since_last_spawn_ >= 1.0) {

            //Make the new actor both responsive, and collidable
            ActorID new_actor = window().stage()->new_actor(mesh_, true, true);
            {
                auto actor = window().stage()->actor(new_actor);

                //Add a cube shape to the collidable
                actor->body().set_mass_box(1.0, 1.0, 1.0, 1.0);
                actor->shape().add_box(1.0, 1.0, 1.0);
                actor->move_to(Vec3(0, 6, -10));
                actor->body().apply_angular_impulse_global(kglt::Vec3(0, 1, 0));
            };

            time_since_last_spawn_ = 0.0;
        }
    }

private:
    kglt::TextureID texture_id_;
    kglt::MeshID mesh_;
    std::vector<ActorID> actors_;

    float time_since_last_spawn_ = 0.0;
};

class BoxDrop: public kglt::Application {
public:
    BoxDrop():
        Application("KGLT BoxDrop Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {
        register_screen("/", screen_factory<GameScreen>());
        return true;
    }
};


int main(int argc, char* argv[]) {
    BoxDrop app;
    return app.run();
}


