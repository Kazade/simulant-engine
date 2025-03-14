#include "simulant/math/bezier_curve.h"
#include "simulant/simulant.h"

using namespace smlt;

class GameScene: public smlt::Scene {
public:
    GameScene(smlt::Window* window) :
        smlt::Scene(window) {}

    void on_load() override {

        camera_ = create_child<smlt::Camera>();
        auto pipeline = compositor->create_layer(this, camera_);

        pipeline->viewport->set_color(smlt::Color::black());

        // Camera
        camera_->set_perspective_projection(
            Degrees(60.0), float(window->width()) / float(window->height()),
            0.01f, 1000.0f);
        camera_->transform->rotate(smlt::Vec3::up(), Degrees(180 - 90));

        smlt::MeshLoadOptions opts;
#ifdef __DREAMCAST__
        opts.override_texture_extension = ".dtex";
#endif

        // Meshes
        cave_mesh_ = app->shared_assets->load_mesh(
            "assets/samples/cave/cave.dcm", VertexSpecification::DEFAULT, opts);
        godray_mesh_ =
            app->shared_assets->load_mesh("assets/samples/cave/godray.obj",
                                          VertexSpecification::DEFAULT, opts);
        fairy_mesh_ =
            app->shared_assets->load_mesh("assets/samples/cave/fairy.obj",
                                          VertexSpecification::DEFAULT, opts);

        // Materials + Textures
        for(auto submesh: cave_mesh_->each_submesh()) {
            submesh->material()->diffuse_map()->set_texture_filter(
                TextureFilter::TEXTURE_FILTER_BILINEAR);
        }

        auto ray_mat = godray_mesh_->first_submesh()->material();
        ray_mat->set_blend_func(BlendType::BLEND_ADD);
        ray_mat->set_lighting_enabled(false);
        ray_mat->pass(0)->set_depth_test_enabled(false);
        ray_mat->pass(0)->set_depth_write_enabled(false);
        ray_mat->diffuse_map()->set_texture_filter(
            TextureFilter::TEXTURE_FILTER_BILINEAR);

        auto fairy_mat = fairy_mesh_->first_submesh()->material();
        fairy_mat->pass(0)->set_blend_func(BlendType::BLEND_ADD);
        fairy_mat->pass(0)->set_lighting_enabled(false);
        fairy_mat->diffuse_map()->set_texture_filter(
            TextureFilter::TEXTURE_FILTER_BILINEAR);

        // Geoms + Actors
        cave_geom_ = create_child<smlt::Actor>(cave_mesh_);
        fairy_actor_ = create_child<smlt::Actor>(fairy_mesh_);
        godray_geom_ = create_child<smlt::Actor>(godray_mesh_);
        fairy_actor_->set_render_priority(RENDER_PRIORITY_FOREGROUND);

        // Lights
        lighting->set_ambient_light(smlt::Color(0.25f, 0.25f, 0.25f, 1.0f));
        create_child<smlt::DirectionalLight>(
            Params()
                .set("direction", Vec3(-120, -90, 0))
                .set("color", Color(1, 0.6822482f, 0.3915094f, 1) * 0.5f));

        Color lightCol = Color(1, 0.6822482f, 0.3915094f, 1.0f);
        auto rock_light = create_child<smlt::PointLight>(
            Params()
                .set("position", Vec3(-12.15f, -0.67f, 0.73f))
                .set("color", lightCol * 23.0f));

        rock_light->set_attenuation(4.31f, 0.01f, 0.25f, 0.75);

        auto fairy_light = create_child<smlt::PointLight>(
            Params()
                .set("position", Vec3())
                .set("color", Color(0.5f, 0.85f, 1, 1) * 10));
        fairy_light->set_attenuation(5, 0.01f, 0.25f, 0.75f);
        fairy_light->set_parent(fairy_actor_);

        // BezierPath
        Vec3 p0 = Vec3(-9.96f, 0.8f, 5.2f);
        Vec3 p1 = Vec3(-7.779675f, -1, -18.46234f);
        Vec3 p2 = Vec3(0.0f, -2, 5.28f);
        Vec3 p3 = p0;

        fairyPath_ = new BezierCurve(p0, p1, p2, p3);

        // Fairy
        fairy_actor_->transform->set_position(fairyPath_->calc_bezier_point(0));

        panel_cam_ = create_child<Camera>();
        panel_cam_->set_orthographic_projection(0, window->width(), 0,
                                                window->height());
        stats_ = create_child<StatsPanel>();
        stats_->activate();

        sound_ = assets->load_sound("assets/samples/cave/ambient.wav");
        compositor->create_layer(stats_, panel_cam_,
                                 smlt::RENDER_PRIORITY_FOREGROUND);

        debug_ = scene->create_child<Debug>();
    }

    void on_update(float dt) override {
        _S_UNUSED(dt);

        if(input->axis_value_hard("Start") == 1) {
            app->stop_running();
        }
    }

    void on_fixed_update(float dt) override {

        // Move the camera between two points
        camera_->transform->set_position(camera_->transform->position() +
                                         Vec3::left() * cameraSpeed_ * dt);
        if(camera_->transform->position().x > 0 ||
           camera_->transform->position().x < -15) {
            cameraSpeed_ *= -1;
        }

        // Always look at the fairy
        camera_->transform->look_at(fairy_actor_->transform->position());

        // // Also make sure the fairy is always at the camera (billboard)
        // Vec3 dir = (camera_->transform->position() -
        // fairy_actor_->transform->position()).normalized(); Quaternion rot =
        // smlt::Vec3::NEGATIVE_Z.rotation_to(dir);

        // // Offset the rotation on the UP axis by 90 degrees
        // fairy_actor_->transform->set_orientation(rot * Quaternion(Vec3::UP,
        // Degrees(90)));
        fairy_actor_->transform->look_at(camera_->transform->position());
        fairy_actor_->transform->rotate(Vec3::up(), Degrees(90));

        // Make sure the fairy loops the path (t ranges from 0 to 1)
        if(fairyPathTime_ >= 1) {
            fairyPathTime_ = 0;
        }

        // Get the last position for the upcoming step size calculation
        Vec3 prevFairyPos = fairy_actor_->transform->position();

        // Move the fairy across the path to the new position
        auto fairyPos = fairyPath_->calc_bezier_point(fairyPathTime_);
        fairy_actor_->transform->set_position(fairyPos);

        // Make sure the fairy moves at a constant speed
        lastFairyPathStepSize = fairyPos.squared_distance_to(prevFairyPos);

        if(lastFairyPathStepSize < fairyPathStepSize_) {
            fairyPathSpeedFactor_ *= 1.1f;
        } else {
            fairyPathSpeedFactor_ *= 0.9f;
        }

        fairyPathTime_ += fairyPathSpeedFactor_ * dt;
    }

private:
    CameraPtr camera_;

    Debug* debug_ = nullptr;

    SoundPtr sound_;
    PlayingSoundPtr player_;

    MeshPtr cave_mesh_;
    MeshPtr godray_mesh_;
    MeshPtr fairy_mesh_;

    ActorPtr cave_geom_;
    ActorPtr godray_geom_;
    ActorPtr fairy_actor_;

    float cameraSpeed_ = 0.35f;
    BezierCurve* fairyPath_ = nullptr;
    float fairyPathTime_ = 0;
    const float fairyPathSpeed_ = 0.75f;

    float fairyPathSpeedFactor_ = fairyPathSpeed_ / 10;
    const float fairyPathStepSize_ =
        (fairyPathSpeed_ / 60) * (fairyPathSpeed_ / 60);
    float lastFairyPathStepSize;

    StatsPanel* stats_ = nullptr;
    CameraPtr panel_cam_;
};

class CaveDemo: public smlt::Application {
public:
    CaveDemo(const AppConfig& config) :
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
    config.log_level = smlt::LOG_LEVEL_INFO;
    config.development.force_renderer = "gl1x";

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
