#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/extra/skybox.h"

using namespace kglt;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(WindowBase& window):
        kglt::Screen<GameScreen>(window) {}

    void do_load() {
        unicode filename = ""; //FIXME: Figure out how to pass this down to the screen!

        prepare_basic_scene(stage_id_, camera_id_);

        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);

        stage->set_ambient_light(kglt::Colour(1.0, 1.0, 1.0, 1.0));
        window->camera(camera_id_)->set_perspective_projection(45.0, float(window->width()) / float(window->height()));

        kglt::MeshID mid = stage->new_mesh_from_file(filename);
        kglt::ActorID actor_id = stage->new_actor(mid);

        {
            auto actor = stage->actor(actor_id);
            actor->set_absolute_position(0, 0, -30);
        }

        //Load a particle system and attach it to the actor
        auto ps_id = stage->new_particle_system_with_parent_from_file(actor_id, "kglt/particles/rocket_trail.kglp");
        stage->particle_system(ps_id)->move_to(0, 0, -10);

        //Just stash the skybox with the window so we always have access to it
        window->data->stash(kglt::extra::StarField::create(stage), "skybox");

        //Set the camera to follow the model
        stage->camera(camera_id_)->follow(actor_id, kglt::Vec3(0, 5, 50));


        //Connect keyboard signals
        window->keyboard().key_while_pressed_connect(SDL_SCANCODE_LEFT, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_y(kglt::Degrees(-20.0 * dt));
        });

        window->keyboard().key_while_pressed_connect(SDL_SCANCODE_RIGHT, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_y(kglt::Degrees(20.0 * dt));
        });

        window->keyboard().key_while_pressed_connect(SDL_SCANCODE_UP, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_x(kglt::Degrees(20.0 * dt));
        });

        window->keyboard().key_while_pressed_connect(SDL_SCANCODE_DOWN, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_x(kglt::Degrees(-20.0 * dt));
        });

        window->keyboard().key_while_pressed_connect(SDL_SCANCODE_A, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_z(kglt::Degrees(-60.0 * dt));
        });

        window->keyboard().key_while_pressed_connect(SDL_SCANCODE_D, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_z(kglt::Degrees(60.0 * dt));
        });

        window->keyboard().key_while_pressed_connect(SDL_SCANCODE_S, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->move_forward(60 * dt);
        });
    }
private:
    CameraID camera_id_;
    StageID stage_id_;
};

class FlightSimSample: public kglt::Application {
public:
    FlightSimSample(const unicode& path):
        Application("KGLT Combined Sample") {
    }

private:
    bool do_init() {
        register_screen("/", screen_factory<GameScreen>());
        return true;
    }
};

int main(int argc, char* argv[]) {
    unicode filename;
    if(argc < 2) {
        filename = "sample_data/fighter_good/space_frigate_6.obj";
    } else {
        filename = argv[1];
    }
    filename = filename.lower();

    if(!filename.ends_with(".opt") && !filename.ends_with(".obj")) {
        std::cout << "Please specify a file with a .opt or .obj extension" << std::endl;
        return 2;
    }

    FlightSimSample app(filename);
    return app.run();
}



