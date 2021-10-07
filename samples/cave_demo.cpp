#include "simulant/simulant.h"
#include "simulant/math/bezier_curve.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(smlt::Window* window):
        smlt::Scene<GameScene>(window) {}

        void load() {
            stage_ = window->new_stage(smlt::PARTITIONER_NULL);
            camera_ = stage_->new_camera();
            auto pipeline = compositor->render(
            stage_, camera_
        );
        link_pipeline(pipeline);

        pipeline->viewport->set_colour(smlt::Colour::BLACK);

        // Camera
        camera_->set_perspective_projection(Degrees(60.0), float(window->width()) / float(window->height()), 0.01f, 1000.0f);
        camera_->rotate_global_y_by(Degrees(180 - 90));

        // Meshes
        cave_mesh_ = window->shared_assets->new_mesh_from_file("sample_data/cave/cave.obj");
        godray_mesh_ = window->shared_assets->new_mesh_from_file("sample_data/cave/godray.obj");
        fairy_mesh_ = window->shared_assets->new_mesh_from_file("sample_data/cave/fairy.obj");

        // Materials + Textures
        for(auto submesh : cave_mesh_->each_submesh())
            submesh->material()->diffuse_map()->set_texture_filter(TextureFilter::TEXTURE_FILTER_BILINEAR);

        auto ray_mat = godray_mesh_->first_submesh()->material();
        ray_mat->set_blend_func(BlendType::BLEND_ADD);
        ray_mat->set_lighting_enabled(false);
        ray_mat->pass(0)->set_depth_test_enabled(false);
        ray_mat->pass(0)->set_depth_write_enabled(false);
        ray_mat->diffuse_map()->set_texture_filter(TextureFilter::TEXTURE_FILTER_BILINEAR);

        auto fairy_mat = fairy_mesh_->first_submesh()->material();
        fairy_mat->pass(0)->set_blend_func(BlendType::BLEND_ADD);
        fairy_mat->pass(0)->set_lighting_enabled(false);
        fairy_mat->diffuse_map()->set_texture_filter(TextureFilter::TEXTURE_FILTER_BILINEAR);

        // Geoms + Actors
        cave_geom_ = stage_->new_geom_with_mesh(cave_mesh_);
        fairy_actor_ = stage_->new_actor_with_mesh(fairy_mesh_);
        godray_geom_ = stage_->new_geom_with_mesh(godray_mesh_);
        fairy_actor_->set_render_priority(10);

        // Lights
        stage_->set_ambient_light(smlt::Colour(0.25f, 0.25f, 0.25f, 1.0f));
        stage_->new_light_as_directional(Vec3(-120, -90, 0), Colour(1, 0.6822482f, 0.3915094f, 1) * 0.5f);

        Colour lightCol = Colour(1, 0.6822482f, 0.3915094f, 1.0f);
        auto rock_light = stage_->new_light_as_point(Vec3(-12.15f, -0.67f, 0.73f), lightCol * 23.0f);
        rock_light->set_attenuation(4.31f, 0.01f, 0.25f, 0.75);

        auto fairy_light = stage_->new_light_as_point(Vec3(), Colour(0.5f, 0.85f, 1, 1) * 10);
        fairy_light->set_attenuation(5, 0.01f, 0.25f, 0.75f);
        fairy_light->set_parent(fairy_actor_);

        // BezierPath
        Vec3 p0 = Vec3(-9.96f, 0.8f, 5.2f);
        Vec3 p1 = Vec3(-7.779675f, -1, -18.46234f);
        Vec3 p2 = Vec3(0.0f, -2, 5.28f);
        Vec3 p3 = p0;

        fairyPath_ = new BezierCurve(p0, p1, p2, p3);

        // Fairy
        fairy_actor_->move_to_absolute(fairyPath_->calc_bezier_point(0));
    }

    void update(float dt) override {
        _S_UNUSED(dt);

        if(input->axis_value_hard("Start") == 1) {
            window->stop_running();
        }
    }

    void fixed_update(float dt) override {

        // Move the camera between two points
        camera_->move_to(camera_->position() + Vec3::LEFT * cameraSpeed_ * dt);
        if(camera_->absolute_position().x > 0 || camera_->absolute_position().x < -15)
            cameraSpeed_ *= -1;

        // Always look at the fairy
        camera_->look_at(fairy_actor_->position());

        // Also make sure the fairy is always at the camera (billboard)
        Vec3 dir = (camera_->position() - fairy_actor_->position()).normalized();
        Quaternion rot = smlt::Vec3::NEGATIVE_Z.rotation_to(dir);

        // Offset the rotation on the UP axis by 90 degrees
        fairy_actor_->rotate_to(rot * Quaternion(Vec3::UP, Degrees(90)));

        // Make sure the fairy loops the path (t ranges from 0 to 1)
        if(fairyPathTime_ >= 1)
            fairyPathTime_ = 0;

        // Get the last position for the upcoming step size calculation
        Vec3 prevFairyPos = fairy_actor_->absolute_position();

        // Move the fairy across the path to the new position
        fairy_actor_->move_to_absolute(fairyPath_->calc_bezier_point(fairyPathTime_));

        // Make sure the fairy moves at a constant speed
        lastFairyPathStepSize = fairy_actor_->absolute_position().sqr_distance(fairy_actor_->absolute_position(), prevFairyPos);

        if(lastFairyPathStepSize < fairyPathStepSize_) {
            fairyPathSpeedFactor_ *= 1.1f;
        }  else {
            fairyPathSpeedFactor_ *= 0.9f;
        }

        fairyPathTime_ += fairyPathSpeedFactor_ * dt;
    }

private:
    StagePtr stage_;
    CameraPtr camera_;

    MeshPtr cave_mesh_;
    MeshPtr godray_mesh_;
    MeshPtr fairy_mesh_;

    GeomPtr cave_geom_;
    GeomPtr godray_geom_;
    ActorPtr fairy_actor_;

    float cameraSpeed_ = 0.35f;
    BezierCurve* fairyPath_ = nullptr;
    float fairyPathTime_ = 0;
    const float fairyPathSpeed_ = 0.75f;

    float fairyPathSpeedFactor_ = fairyPathSpeed_ / 10;
    const float fairyPathStepSize_ = (fairyPathSpeed_ / 60) * (fairyPathSpeed_ / 60);
    float lastFairyPathStepSize;
};


class CaveDemo: public smlt::Application {
public:
    CaveDemo(const AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        return true;
    }
};


int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "Cave Demo";
    config.fullscreen = false;

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    CaveDemo app(config);
    return app.run();
}
