
#include <simulant/simulant.h>

class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::WindowBase& window):
        smlt::Scene<MainScene>(window) {}

    void do_load() {
        prepare_basic_scene(stage_, camera_);

        smlt::StagePtr stage = stage_.fetch();

        auto ps = stage->new_particle_system_from_file("simulant/particles/fire.kglp").fetch();
        ps->move_to(0, 0, -50);

        auto mat = stage->assets->new_material_from_file(smlt::Material::BuiltIns::TEXTURED_PARTICLE).fetch();
        ps->set_material_id(mat->id());
        mat->set_texture_unit_on_all_passes(0, stage->assets->new_texture_from_file("sample_data/flare.tga"));

        window->camera(camera_)->set_perspective_projection(
            smlt::Degrees(45.0),
            float(window->width()) / float(window->height()),
            0.1,
            1000.0
        );

        L_DEBUG("Scene loaded");
    }

private:
    smlt::StageID stage_;
    smlt::CameraID camera_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {

        window->set_logging_level(smlt::LOG_LEVEL_DEBUG);
    }

    bool do_init() {
        register_scene<MainScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "Particles";
    App app(config);
    return app.run();
}
