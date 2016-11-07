#include "simulant/utils/random.h"

#include "simulant/simulant.h"

using namespace smlt;

class GameScreen : public smlt::Screen<GameScreen> {
public:
    GameScreen(smlt::WindowBase& window):
        smlt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        pipeline_id_ = prepare_basic_scene(stage_id_, camera_id_, smlt::PARTITIONER_NULL);
        window->disable_pipeline(pipeline_id_);

        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);
        window->camera(camera_id_)->set_perspective_projection(
            45.0, float(window->width()) / float(window->height()), 1.0, 1000.0
        );

        stage->camera(camera_id_)->move_to(0, 10, 50);

        window->pipeline(pipeline_id_)->viewport->set_colour(smlt::Colour::SKY_BLUE);

        smlt::TextureID crate = window->shared_assets->new_texture_from_file("sample_data/crate.png");
        smlt::MaterialID mat = window->shared_assets->new_material_from_texture(crate);

        box_mesh_id_ = window->shared_assets->new_mesh_as_box(5, 2.5, 7);
        window->shared_assets->mesh(box_mesh_id_)->set_material_id(mat);
        object_id_ = stage->new_actor_with_mesh(box_mesh_id_);

        smlt::TextureID grass = window->shared_assets->new_texture_from_file("sample_data/beach_sand.png");
        ground_mesh_id_ = window->shared_assets->new_mesh_as_box(1000, 2.5, 1000); //window->shared_assets->new_mesh_from_file("sample_data/playground.obj");
        window->shared_assets->mesh(ground_mesh_id_)->set_material_id(
            window->shared_assets->new_material_from_texture(grass)
        );
        ground_id_ = stage->new_actor_with_mesh(ground_mesh_id_);

        auto object = stage->actor(object_id_);

        stage->camera(camera_id_)->follow(object_id_, CAMERA_FOLLOW_MODE_DIRECT);

        // Create a rigid body simulation
        simulation_ = controllers::RigidBodySimulation::create();

        // Add a rigid body controller to the object and store it
        controller_ = object->new_controller<controllers::RaycastVehicle>(simulation_, 1.0);
        controller_->move_to(Vec3(0, 10, 0));

        // Make the ground a staticbody, and only deal with ray-cast hits
        auto ground_controller = stage->actor(ground_id_)->new_controller<controllers::StaticBody>(simulation_, controllers::COLLIDER_TYPE_RAYCAST_ONLY);
        //ground_controller->move_to(Vec3(0, -10, 0));
    }

    void do_activate() {
        window->enable_pipeline(pipeline_id_);


        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_UP, [=](SDL_Keysym key, double dt) mutable {
            controller_->accelerate();
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_DOWN, [=](SDL_Keysym key, double dt) mutable {
            controller_->decelerate();
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_RIGHT, [=](SDL_Keysym key, double dt) mutable {
            controller_->turn_left();
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_LEFT, [=](SDL_Keysym key, double dt) mutable {
            controller_->turn_right();
        });
    }

    void do_step(double dt) override {
        simulation_->step(dt);
    }

private:
    PipelineID pipeline_id_;
    StageID stage_id_;
    CameraID camera_id_;

    MeshID box_mesh_id_;
    ActorID object_id_;

    MeshID ground_mesh_id_;
    ActorID ground_id_;

    controllers::RigidBodySimulation::ptr simulation_;
    controllers::RaycastVehicle::ptr controller_;
};


class PhysicsDemo: public smlt::Application {
public:
    PhysicsDemo():
        smlt::Application("Physics Demo") {}

private:
    bool do_init() {
        register_screen("/", smlt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    PhysicsDemo app;
    return app.run();
}
