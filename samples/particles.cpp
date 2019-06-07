
#include <simulant/simulant.h>

class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::Window* window):
        smlt::Scene<MainScene>(window) {}

    void load() {
        auto pipeline = prepare_basic_scene(stage_, camera_, smlt::PARTITIONER_NULL);
        pipeline->viewport->set_colour(smlt::Colour::GREY);
        pipeline->set_clear_flags(~0);

        if(!app->args->arg_value<bool>("stress", false).value()) {
            auto path = app->args->arg_value<std::string>("filename");
            ps_ = stage_->new_particle_system_from_file(
                !path.has_value() ? "simulant/particles/fire.kglp" : path.value()
            );

            ps_->move_to(0.0, 0, -4);
            ps_->set_render_priority(smlt::RENDER_PRIORITY_MAIN + 1);
            ps_->material_id().fetch()->pass(0)->set_blend_func(smlt::BLEND_NONE);
        } else {
            /* Generate 1024 particle system instances in a grid */
            for(auto z = -16; z < 16; ++z) {
                for(auto x = -16; x < 16; ++x) {
                    auto path = app->args->arg_value<std::string>("filename", "simulant/particles/fire.kglp");
                    auto ps = stage_->new_particle_system_from_file(
                        path.value()
                    );

                    ps->move_to(x * 5, 0, z * 5);
                }
            }
        }


        camera_->set_perspective_projection(
            smlt::Degrees(45.0),
            float(window->width()) / float(window->height()),
            0.1,
            1000.0
        );

        auto fly = camera_->new_behaviour<smlt::behaviours::Fly>(window);
        fly->set_speed(10.0f);

        L_DEBUG("Scene loaded");
    }

    void update(float dt) override {
        ps_->rotate_global_y_by(smlt::Degrees(90.0f * dt));
    }

private:
    smlt::StagePtr stage_;
    smlt::CameraPtr camera_;
    smlt::ParticleSystemPtr ps_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {

        args->define_arg("--stress", smlt::ARG_TYPE_BOOLEAN, "stress test the particle system");
        args->define_arg("--filename", smlt::ARG_TYPE_STRING, "display the selected file");
        window->set_logging_level(smlt::LOG_LEVEL_DEBUG);
    }

    bool init() {
        scenes->register_scene<MainScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Particles";
    config.fullscreen = false;
    config.width = 1024;
    config.height = 768;

    App app(config);
    return app.run(argc, argv);
}
