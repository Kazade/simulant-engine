#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"

using namespace smlt;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        prepare_basic_scene(stage_, camera_);

        camera_->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            0.1,
            1000.0
        );

        stage_->set_ambient_light(smlt::Colour(0.2, 0.2, 0.2, 1.0));

        actor_ = stage_->new_actor_with_mesh(stage_->assets->new_mesh_as_cube(2.0));
        actor_->move_to(0.0, 0.0, -5.0);

        texture_ = stage_->assets->new_texture_from_file("sample_data/crate.png");
        texture_.fetch()->set_texture_filter(TEXTURE_FILTER_BILINEAR);

        actor_->mesh()->set_texture_on_material(0, texture_);

        // Test Camera::look_at function
        camera_->look_at(actor_->absolute_position());

        {
            auto light = stage_->new_light_as_point(Vec3(5, 0, -5), smlt::Colour::GREEN);
            light->set_attenuation_from_range(20.0);

            auto light2 = stage_->new_light_as_point(Vec3(-5, 0, -5), smlt::Colour::BLUE);
            light2->set_attenuation_from_range(30.0);

            auto light3 = stage_->new_light_as_point(Vec3(0, -15, -5), smlt::Colour::RED);
            light3->set_attenuation_from_range(50.0);

            stage_->new_light_as_directional(Vec3(1, 0, 0), smlt::Colour::YELLOW);
        }

        window->new_background_as_scrollable_from_file("sample_data/background.png");        

        auto axis = input->new_axis("F");
        axis->set_positive_keyboard_key(smlt::KEYBOARD_CODE_F);
    }

    void update(float dt) {
        if(input->axis_value_hard("F") == 1 && !f_down) {
            current_filter_++;
            if(current_filter_ == 3) {
                current_filter_ = 0;
            }

            texture_.fetch()->set_texture_filter(filters_[current_filter_]);
            f_down = true;
        } else if(input->axis_value_hard("F") == 0) {
            f_down = false;
        }
    }

    void fixed_update(float dt) {
        actor_->rotate_global_y_by(smlt::Degrees(input->axis_value("Horizontal") * 360.0f * dt));

        actor_->rotate_x_by(smlt::Degrees(dt * 20.0));
        actor_->rotate_y_by(smlt::Degrees(dt * 15.0));
        actor_->rotate_z_by(smlt::Degrees(dt * 25.0));
    }

private:
    CameraPtr camera_;
    StagePtr stage_;
    ActorPtr actor_;
    TextureID texture_;

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
    smlt::AppConfig config;
    config.title = "Light Sample";
    config.fullscreen = false;
    config.width = 1280;
    config.height = 960;

    LightingSample app(config);
    return app.run();
}

