#include "simulant/simulant.h"

#include <cstdlib>
#include <ctime>

using namespace smlt;

class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window):
        smlt::Scene(window) {}

    void on_load() override {
        start_service<PhysicsService>();

        camera_ = create_child<smlt::Camera>();
        pipeline_ = compositor->create_layer(
            this, camera_
        );

        pipeline_->viewport->set_color(smlt::Color::blue());
        pipeline_->set_clear_flags(smlt::BUFFER_CLEAR_ALL);

        camera_->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()), 1.0, 1000.0
        );

        camera_->transform->set_position(Vec3(0, 10, 50));

        // Create a nice skybox (not on DC, the image is too big)
        if(get_platform()->name() != "dreamcast") {
            create_child<Skybox>("assets/samples/skyboxes/TropicalSunnyDay");
        }

        auto crate =
            app->shared_assets->load_texture("assets/samples/crate.png");
        auto mat = app->shared_assets->create_material_from_texture(crate);

        auto box_mesh = app->shared_assets->create_mesh(smlt::VertexSpecification::DEFAULT, smlt::GARBAGE_COLLECT_NEVER);
        box_mesh->create_submesh_as_cube("cube", mat, 5);
        box_mesh_ = box_mesh;

        auto grass =
            app->shared_assets->load_texture("assets/samples/beach_sand.png");
        auto ground_mesh = app->shared_assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        ground_mesh->create_submesh_as_box(
            "ground", app->shared_assets->create_material_from_texture(grass), 1000, 2.5, 1000
        ); //window->shared_assets->load_mesh("sample_data/playground.obj");

        ground_mesh_ = ground_mesh;
        ground_ = create_child<smlt::Actor>(ground_mesh_);

        // Make the ground a staticbody
        auto c = ground_->create_mixin<smlt::StaticBody>();
        c->add_box_collider(ground_->aabb().dimensions(),
                            PhysicsMaterial::stone());
    }

    void spawn_box() {
        auto box = create_child<smlt::Actor>(box_mesh_);
        auto pos = Vec3(
            smlt::RandomGenerator::instance().float_in_range(-20.0f, 20.0f),
            smlt::RandomGenerator::instance().float_in_range(20.0f, 30.0f),
            smlt::RandomGenerator::instance().float_in_range(-10.0f, 10.0f));

        auto controller = create_child<smlt::DynamicBody>(pos);
        controller->add_box_collider(box->aabb().dimensions(),
                                     PhysicsMaterial::wood());

        box->set_parent(controller);
        boxes_.push_back(controller);
        controller->transform->set_position(pos);
    }

    void on_update(float dt) override {
        counter += dt;
        if(counter > 1.0f) {
            counter = 0;
            spawn_box();

            if(boxes_.size() > 10) {
                boxes_.front()->destroy();
                boxes_.erase(boxes_.begin());
            }
        }
    }

private:
    std::vector<StageNodePtr> boxes_;

    LayerPtr pipeline_;
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
