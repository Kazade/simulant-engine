#include "simulant/simulant.h"

#include <cstdlib>
#include <ctime>

using namespace smlt;

class GameScene : public smlt::PhysicsScene<GameScene> {
public:
    GameScene(smlt::Window* window):
        smlt::PhysicsScene<GameScene>(window) {}

    void load() {
        stage_ = new_stage(smlt::PARTITIONER_NULL);
        camera_ = stage_->new_camera();
        pipeline_ = compositor->render(
            stage_, camera_
        );
        link_pipeline(pipeline_);

        pipeline_->viewport->set_colour(smlt::Colour::BLUE);

        camera_->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()), 1.0, 1000.0
        );

        camera_->move_to(0, 10, 50);

        // Create a nice skybox (not on DC, the image is too big)
        if(get_platform()->name() != "dreamcast") {
            stage_->skies->new_skybox_from_folder("sample_data/skyboxes/TropicalSunnyDay");
        }

        smlt::TextureID crate = app->shared_assets->new_texture_from_file("sample_data/crate.png");
        smlt::MaterialID mat = app->shared_assets->new_material_from_texture(crate);

        auto box_mesh = app->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT, smlt::GARBAGE_COLLECT_NEVER);
        box_mesh->new_submesh_as_cube("cube", mat, 5);
        box_mesh_id_ = box_mesh;

        smlt::TextureID grass = app->shared_assets->new_texture_from_file("sample_data/beach_sand.png");
        auto ground_mesh = app->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        ground_mesh->new_submesh_as_box(
            "ground", app->shared_assets->new_material_from_texture(grass), 1000, 2.5, 1000
        ); //window->shared_assets->new_mesh_from_file("sample_data/playground.obj");

        ground_mesh_id_ = ground_mesh;
        ground_ = stage_->new_actor_with_mesh(ground_mesh_id_);

        // Make the ground a staticbody
        auto c = ground_->new_behaviour<behaviours::StaticBody>(physics);
        c->add_box_collider(ground_->aabb().dimensions(), behaviours::PhysicsMaterial::STONE);

        srand(time(nullptr));
    }

    void spawn_box() {
        boxes_.push_back(
            stage_->new_actor_with_mesh(box_mesh_id_)
        );

        auto box = boxes_.back();
        auto controller = box->new_behaviour<smlt::behaviours::RigidBody>(physics);
        controller->add_box_collider(box->aabb().dimensions(), behaviours::PhysicsMaterial::WOOD);
        controller->move_to(Vec3(
            ((float(rand()) / RAND_MAX) * 20.0f) - 10.0f,
            20, 0)
        );
    }

    void update(float dt) {
        counter += dt;
        if(counter > 1.0f) {
            counter = 0;
            spawn_box();
        }
    }

private:
    std::vector<ActorPtr> boxes_;

    PipelinePtr pipeline_;
    StagePtr stage_;
    CameraPtr camera_;

    MeshID box_mesh_id_;

    MeshID ground_mesh_id_;
    ActorPtr ground_;

    float counter = 0.0f;
};


class PhysicsDemo: public smlt::Application {
public:
    PhysicsDemo(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        scenes->activate("_loading"); // Show the loading screen in the meantime
        scenes->preload_in_background("main").then([this]() {
            scenes->activate("main");
        }); //Do loading in a background thread, but show immediately when done
        return true;
    }
};


int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Physics Sample";

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
#endif

    PhysicsDemo app(config);
    return app.run();
}
