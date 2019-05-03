
#include <simulant/simulant.h>

std::string passed_filename;
bool stress_test = false;

class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::Window* window):
        smlt::Scene<MainScene>(window) {}

    void load() {
        auto pipeline = prepare_basic_scene(stage_, camera_);
        pipeline->viewport->set_colour(smlt::Colour::GREY);
        pipeline->set_clear_flags(~0);

        if(!stress_test) {
            auto ps = stage_->new_particle_system_from_file(
                passed_filename.empty() ? "simulant/particles/fire.kglp" : passed_filename
            );

            ps->move_to(0.0, 0, -4);
            ps->set_render_priority(smlt::RENDER_PRIORITY_MAIN + 1);
        } else {
            /* Generate 1024 particle system instances in a grid */
            for(auto z = -16; z < 16; ++z) {
                for(auto x = -16; x < 16; ++x) {
                    auto ps = stage_->new_particle_system_from_file(
                        passed_filename.empty() ? "simulant/particles/fire.kglp" : passed_filename
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

        L_DEBUG("Scene loaded");
    }

private:
    smlt::StagePtr stage_;
    smlt::CameraPtr camera_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {

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

    if(argc > 1) {
        passed_filename = argv[1];
    }

    for(auto i = 0; i < argc; ++i) {
        if(std::string(argv[i]) == "--stress") {
            stress_test = true;
            break;
        }
    }

    App app(config);
    return app.run();
}
