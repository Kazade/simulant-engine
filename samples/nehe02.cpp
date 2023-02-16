
#include <simulant/simulant.h>
#include <simulant/utils/dreamcast.h>

class MainScene : public smlt::Scene<MainScene> {
public:
    MainScene(smlt::Window* window):
        smlt::Scene<MainScene>(window) {}

    void load() override {
        stage_ = new_stage(smlt::PARTITIONER_NULL);
        camera_ = stage_->new_camera();
        auto pipeline = compositor->render(
            stage_, camera_
        );
        link_pipeline(pipeline);

        pipeline->viewport->set_colour(smlt::Colour::RED);

        smlt::MeshPtr square = stage_->assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        square->new_submesh_as_rectangle("rect", stage_->assets->new_material(), 1.0, 1.0);

        auto actor = stage_->new_actor_with_mesh(square->id());
        actor->move_to(0, 0, -5);
        actor->scale_by(2.0);
        S_DEBUG("Scene loaded");


        auto tex = stage_->assets->new_texture_from_file("simulant/textures/icons/simulant-icon-vmu.png");
        tex->convert(smlt::TEXTURE_FORMAT_RGBA_4UB_8888, {smlt::TEXTURE_CHANNEL_INVERSE_RED, smlt::TEXTURE_CHANNEL_INVERSE_RED, smlt::TEXTURE_CHANNEL_INVERSE_RED, smlt::TEXTURE_CHANNEL_INVERSE_RED});
        auto data_maybe = smlt::utils::vmu_lcd_image_from_texture(tex, smlt::utils::VMU_IMAGE_GENERATION_MODE_ALPHA);
        data = data_maybe.value();

        /* Render the simulant icon to the VMU */
    }

    void update(float dt) override {
        _S_UNUSED(dt);

        window->each_screen([=](std::string, smlt::Screen* screen) {
            screen->render(&data[0]);
        });

        auto controller = window->input->state->game_controller(smlt::GameControllerIndex(0));
        if(controller) {
            controller->start_rumble(0.75f, 0, smlt::Seconds(1.0f));
        }
    }

private:
    std::vector<uint8_t> data;
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
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "NeHe 02";
    config.fullscreen = false;

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    App app(config);
    return app.run();
}
