#include "simulant/simulant.h"

#include <cstdlib>
#include <ctime>

using namespace smlt;

class GameScene : public smlt::PhysicsScene<GameScene> {
public:
    GameScene(smlt::Window* window):
        smlt::PhysicsScene<GameScene>(window) {}

    void load() {
        stage_ = window->new_stage(smlt::PARTITIONER_NULL);
        camera_ = stage_->new_camera();
        pipeline_ = compositor->render(
            stage_, camera_
        );
        link_pipeline(pipeline_);

        pipeline_->viewport->set_colour(smlt::Colour::SKY_BLUE);

        camera_->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()), 1.0, 1000.0
        );

        camera_->move_to(0, 10, 50);

        // Create a nice skybox (not on DC, the image is too big)
        if(window->platform->name() != "dreamcast") {
            stage_->skies->new_skybox_from_folder("sample_data/skyboxes/TropicalSunnyDay");
        }

        smlt::TextureID crate = window->shared_assets->new_texture_from_file("sample_data/crate.png");
        smlt::MaterialID mat = window->shared_assets->new_material_from_texture(crate);

        auto box_mesh = window->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT, smlt::GARBAGE_COLLECT_NEVER);
        box_mesh->new_submesh_as_cube("cube", mat, 5);
        box_mesh_id_ = box_mesh;

        smlt::TextureID grass = window->shared_assets->new_texture_from_file("sample_data/beach_sand.png");
        auto ground_mesh = window->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        ground_mesh->new_submesh_as_box(
            "ground", window->shared_assets->new_material_from_texture(grass), 1000, 2.5, 1000
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
        scenes->load_in_background("main", true); //Do loading in a background thread, but show immediately when done
        scenes->activate("_loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Physics Sample";

    PhysicsDemo app(config);
    return app.run();
}
