#include "simulant/simulant.h"
#include "simulant/shortcuts.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(WindowBase& window):
        smlt::Scene<GameScene>(window, "game_screen") {}

    void do_load() {
        unicode filename = window->application->data->get<unicode>("filename");

        prepare_basic_scene(stage_id_, camera_id_);

        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);

        stage->set_ambient_light(smlt::Colour(1.0, 1.0, 1.0, 1.0));
        window->camera(camera_id_)->set_perspective_projection(45.0, float(window->width()) / float(window->height()));

        smlt::MeshID mid = stage->assets->new_mesh_from_file(filename);
        smlt::ActorID actor_id = stage->new_actor_with_mesh(mid);

        {
            auto actor = stage->actor(actor_id);
            actor->move_to_absolute(0, 0, -30);
        }

        //Load a particle system and attach it to the actor
        auto ps_id = stage->new_particle_system_with_parent_from_file(actor_id, "simulant/particles/pixel_trail.kglp");
        stage->particle_system(ps_id)->move_to(0, 0, -10);

        //Just stash the skybox with the window so we always have access to it
//        window->data->stash(smlt::extra::StarField::create(stage, camera_id_), "skybox");

        //Set the camera to follow the model
        stage->camera(camera_id_)->follow(actor_id, smlt::CAMERA_FOLLOW_MODE_DIRECT, smlt::Vec3(0, 5, 50));


        //Connect keyboard signals
        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_LEFT, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_y_by(smlt::Degrees(-20.0 * dt));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_RIGHT, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_y_by(smlt::Degrees(20.0 * dt));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_UP, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_x_by(smlt::Degrees(20.0 * dt));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_DOWN, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_x_by(smlt::Degrees(-20.0 * dt));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_A, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_z_by(smlt::Degrees(-60.0 * dt));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_D, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->rotate_z_by(smlt::Degrees(60.0 * dt));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_S, [=](SDL_Keysym key, double dt) mutable {
            window->stage(stage_id_)->actor(actor_id)->move_forward_by(60 * dt);
        });
    }
private:
    CameraID camera_id_;
    StageID stage_id_;
};

class FlightSimSample: public smlt::Application {
public:
    FlightSimSample():
        Application("Simulant Combined Sample") {
    }

private:
    bool do_init() {
        register_scene<GameScene>("/");
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

    FlightSimSample app;
    app.data->stash(filename, "filename");

    return app.run();
}



