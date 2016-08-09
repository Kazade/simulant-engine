#include <kazbase/random.h>

#include "kglt/kglt.h"

using namespace kglt;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(kglt::WindowBase& window):
        kglt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        pipeline_id_ = prepare_basic_scene(stage_id_, camera_id_, kglt::PARTITIONER_NULL);
        window->disable_pipeline(pipeline_id_);

        auto stage = window->stage(stage_id_);
        stage->host_camera(camera_id_);
        window->camera(camera_id_)->set_perspective_projection(
            45.0, float(window->width()) / float(window->height()), 1.0, 100.0
        );

        window->pipeline(pipeline_id_)->viewport->set_colour(kglt::Colour::SKY_BLUE);

        kglt::TextureID crate = window->shared_assets->new_texture_from_file("sample_data/crate.png");
        kglt::MaterialID mat = window->shared_assets->new_material_from_texture(crate);

        box_mesh_id_ = window->shared_assets->new_mesh_as_box(5, 5, 5);
        window->shared_assets->mesh(box_mesh_id_)->set_material_id(mat);
        object_id_ = stage->new_actor_with_mesh(box_mesh_id_);

        auto object = stage->actor(object_id_);

        // Create a rigid body simulation
        simulation_ = controllers::RigidBodySimulation::create();

        // Add a rigid body controller to the object and store it
        controller_ = object->new_controller<controllers::RigidBody>(simulation_);
        controller_->move_to(Vec3(0, 0, -50));
    }

    void do_activate() {
        window->enable_pipeline(pipeline_id_);

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_SPACE, [=](SDL_Keysym key, double dt) mutable {
            // While space is pressed, add an upward force
            controller_->add_force(Vec3(0, 15, 0));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_A, [=](SDL_Keysym key, double dt) mutable {
            controller_->add_torque(Vec3(0, -15, 0));
        });

        window->keyboard->key_while_pressed_connect(SDL_SCANCODE_D, [=](SDL_Keysym key, double dt) mutable {
            controller_->add_torque(Vec3(0, 15, 0));
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

    controllers::RigidBodySimulation::ptr simulation_;
    controllers::RigidBody::ptr controller_;
};


class PhysicsDemo: public kglt::Application {
public:
    PhysicsDemo():
        kglt::Application("Physics Demo") {}

private:
    bool do_init() {
        register_screen("/", kglt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    PhysicsDemo app;
    return app.run();
}
