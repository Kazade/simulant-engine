#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"
#include "simulant/macros.h"
#include "simulant/utils/dreamcast.h"

using namespace smlt;

class GameScene : public smlt::Scene {
public:
    GameScene(Window* window):
        smlt::Scene(window) {}

    void load() {
        camera_ = create_node<smlt::Camera>();
        auto pipeline = compositor->render(stage_, camera_);
        link_pipeline(pipeline);

        camera_->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            0.1,
            1000.0
        );

        stage_->set_ambient_light(smlt::Colour(0.25f, 0.25f, 0.25f, 1.0f));

        texture_ = assets->new_texture_from_file("sample_data/crate.png");
        texture_->set_texture_filter(TEXTURE_FILTER_BILINEAR);

        material_ = assets->new_material_from_texture(texture_);
        material_->pass(0)->set_lighting_enabled(true);

        auto cube = assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        cube->new_submesh_as_cube("rect", material_, 2.0);
        actor_ = create_node<smlt::Actor>(cube);
        actor_->move_to(0.0, 0.0, -5.0);

        // Test Camera::look_at function
        camera_->look_at(actor_->absolute_position());

        {
            auto light = stage_->new_light_as_point(Vec3(5, 0, -5), smlt::Colour::GREEN);
            light->set_attenuation_from_range(30.0);

            auto light2 = stage_->new_light_as_point(Vec3(-5, 0, -5), smlt::Colour::BLUE);
            light2->set_attenuation_from_range(30.0);

            auto light3 = stage_->new_light_as_point(Vec3(0, -5, -5), smlt::Colour::RED);
            light3->set_attenuation_from_range(30.0);

            stage_->new_light_as_directional(Vec3(1, 0, 0), smlt::Colour::YELLOW);
        }


        auto axis = input->new_axis("F");
        axis->set_positive_keyboard_key(smlt::KEYBOARD_CODE_F);

        auto tex = stage_->assets->new_texture_from_file("simulant/textures/icons/simulant-icon-vmu.png");
        tex->convert(smlt::TEXTURE_FORMAT_RGBA_4UB_8888, {smlt::TEXTURE_CHANNEL_INVERSE_RED, smlt::TEXTURE_CHANNEL_INVERSE_RED, smlt::TEXTURE_CHANNEL_INVERSE_RED, smlt::TEXTURE_CHANNEL_INVERSE_RED});
        auto data_maybe = smlt::utils::vmu_lcd_image_from_texture(tex, utils::VMU_IMAGE_GENERATION_MODE_ALPHA);
        auto data = data_maybe.value();

        /* Render the simulant icon to the VMU */
        window->each_screen([&data](std::string, Screen* screen) {
            if(screen->width() / screen->integer_scale() == 48 && screen->height() / screen->integer_scale() == 32) {
                screen->render(&data[0]);
            }
        });


    }

    void update(float dt) {
        _S_UNUSED(dt);

        if(input->axis_value_hard("F") == 1 && !f_down) {
            current_filter_++;
            if(current_filter_ == 3) {
                current_filter_ = 0;
            }

            texture_->set_texture_filter(filters_[current_filter_]);
            f_down = true;
        } else if(input->axis_value_hard("F") == 0) {
            f_down = false;
        }
    }

    void fixed_update(float dt) {
        actor_->rotate_global_y_by(smlt::Degrees(input->axis_value("Horizontal") * 360.0f * dt));

        actor_->rotate_x_by(smlt::Degrees(dt * 20.0f));
        actor_->rotate_y_by(smlt::Degrees(dt * 15.0f));
        actor_->rotate_z_by(smlt::Degrees(dt * 25.0f));

#if 0
        // Uncomment to test texture scrolling
        material_->diffuse_map()->scroll_x(0.5f * dt);
#endif
    }

private:
    CameraPtr camera_;
    StagePtr stage_;
    ActorPtr actor_;
    TexturePtr texture_;
    MaterialPtr material_;

    bool f_down = false;
    uint8_t current_filter_ = 0;
    const smlt::TextureFilter filters_[3] = {
        TEXTURE_FILTER_POINT,
        TEXTURE_FILTER_BILINEAR,
        TEXTURE_FILTER_TRILINEAR
    };
};

class LightingSample: public smlt::Application {
public:
    LightingSample(const smlt::AppConfig& config):
        Application(config) {
    }

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Light Sample";
    config.fullscreen = false;

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    config.desktop.enable_virtual_screen = true;

    LightingSample app(config);
    return app.run();
}

