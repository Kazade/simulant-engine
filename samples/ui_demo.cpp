
#include <simulant/simulant.h>

class MainScene:
    public smlt::Scene<MainScene> {

public:
    MainScene(smlt::Window* window):
        smlt::Scene<MainScene>(window) {}

    void load() {
        stage_ = window->new_stage();
        camera_ = stage_->new_camera_with_orthographic_projection(0, window->width(), 0, window->height());

        window->render(stage_, camera_).with_clear(smlt::BUFFER_CLEAR_ALL, smlt::Colour::BLACK);

        //stage_->ui->transform_input_with_camera(camera_);

        auto title = stage_->ui->new_widget_as_label("UI Sample demonstrating widgets");
        title->move_to(window->coordinate_from_normalized(0.5, 0.9));

        auto button = stage_->ui->new_widget_as_button("Button 1");
        button->move_to(window->coordinate_from_normalized(0.1, 0.25));

        auto ttf_font = stage_->assets->new_font_from_ttf("simulant/fonts/orbitron/Orbitron-Bold.ttf", 32);
        auto big_label = stage_->ui->new_widget_as_label("Using a TrueType font!");
        big_label->set_font(ttf_font);
        big_label->move_to(window->coordinate_from_normalized(0.5, 0.6));

        auto pg = stage_->ui->new_widget_as_progress_bar();
        pg->move_to(window->coordinate_from_normalized(0.5, 0.5));
        pg->resize(400, 10);
        pg->pulse();

        button->signal_clicked().connect([&]() {
            title->set_text("Clicked!");
        });

        auto simulant_logo = stage_->assets->new_texture_from_file("simulant/textures/simulant-icon.png");
        auto icon = stage_->ui->new_widget_as_image(simulant_logo);
        icon->move_to(window->coordinate_from_normalized(0.5, 0.58));
    }

private:
    smlt::StagePtr stage_;
    smlt::CameraPtr camera_;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {

    }

    bool init() {
        scenes->register_scene<MainScene>("main");
        return true;
    }
};

int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "UI Demo";

    App app(config);
    return app.run();
}
