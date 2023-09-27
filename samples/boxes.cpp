#include "simulant/simulant.h"

#include <cstdlib>
#include <ctime>

using namespace smlt;

class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window):
        smlt::Scene(window) {}

    void load() {
        start_service<PhysicsService>();

        camera_ = create_node<smlt::Camera>();
        pipeline_ = compositor->render(
            this, camera_
        );
        link_pipeline(pipeline_);

        pipeline_->viewport->set_colour(smlt::Colour::BLUE);

        camera_->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()), 1.0, 1000.0
        );

        camera_->move_to(0, 10, 50);

        // Create a nice skybox (not on DC, the image is too big)
        if(get_platform()->name() != "dreamcast") {
            create_node<Skybox>("sample_data/skyboxes/TropicalSunnyDay");
        }

        auto crate = app->shared_assets->new_texture_from_file("sample_data/crate.png");
        auto mat = app->shared_assets->new_material_from_texture(crate);

        auto box_mesh = app->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT, smlt::GARBAGE_COLLECT_NEVER);
        box_mesh->new_submesh_as_cube("cube", mat, 5);
        box_mesh_ = box_mesh;

        auto grass = app->shared_assets->new_texture_from_file("sample_data/beach_sand.png");
        auto ground_mesh = app->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        ground_mesh->new_submesh_as_box(
            "ground", app->shared_assets->new_material_from_texture(grass), 1000, 2.5, 1000
        ); //window->shared_assets->new_mesh_from_file("sample_data/playground.obj");

        ground_mesh_ = ground_mesh;
        ground_ = create_node<smlt::Actor>(ground_mesh_);

        // Make the ground a staticbody
        auto c = create_node<smlt::StaticBody>();
        c->adopt_children(ground_); // FIXME: Convert to mixin
        c->add_box_collider(ground_->aabb().dimensions(), PhysicsMaterial::STONE);

        srand(time(nullptr));
    }

    void spawn_box() {
        boxes_.push_back(
            create_node<smlt::Actor>(box_mesh_)
        );

        auto box = boxes_.back();
        auto pos = Vec3(
            ((float(rand()) / RAND_MAX) * 20.0f) - 10.0f, 20, 0
        );

        auto controller = create_node<smlt::RigidBody>(pos);
        controller->add_box_collider(box->aabb().dimensions(), PhysicsMaterial::WOOD);

        box->set_parent(controller);
    }

    void on_update(float dt) override {
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

    MeshPtr box_mesh_;

    MeshPtr ground_mesh_;
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
    config.log_level = LOG_LEVEL_DEBUG;
#endif

    PhysicsDemo app(config);
    return app.run();
}
