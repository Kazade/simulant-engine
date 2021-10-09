
#include <simulant/simulant.h>

class MainScene:
    public smlt::Scene<MainScene> {

public:
    MainScene(smlt::Window* window):
        smlt::Scene<MainScene>(window) {}

    void load() {
        stage_ = window->new_stage();
        camera_ = stage_->new_camera_with_orthographic_projection(0, window->width(), 0, window->height());

        auto pipeline = compositor->render(
            stage_, camera_
        )->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        pipeline->viewport->set_colour(smlt::Colour::BLACK);

        link_pipeline(pipeline);

        auto coord = window->coordinate_from_normalized(0.05, 0.95);
        int x = coord.x;
        int y = coord.y;
        int spacing = 10;
        int column = window->coordinate_from_normalized(0.25, 0).x;

        auto label = stage_->ui->new_widget_as_label("Label");
        label->resize(column, -1);
        label->set_anchor_point(0, 1);
        label->move_to(x, y);

        y -= label->height() + spacing;

        auto button = stage_->ui->new_widget_as_button("Button");
        button->resize(column, -1);
        button->set_anchor_point(0, 1);
        button->move_to(x, y);

        y -= button->height() + spacing;

        pg1_ = stage_->ui->new_widget_as_progress_bar();
        pg1_->set_text("Progress Bar (pulse)");
        pg1_->resize(column, -1);
        pg1_->set_anchor_point(0, 1);
        pg1_->move_to(x, y);

        y -= pg1_->height() + spacing;

        pg2_ = stage_->ui->new_widget_as_progress_bar();
        pg2_->set_text("Progress Bar (percent)");
        pg2_->resize(column, -1);
        pg2_->set_anchor_point(0, 1);
        pg2_->move_to(x, y);

        y -= pg2_->height() + spacing;


        auto added = window->vfs->add_search_path("simulant/fonts/orbitron");
        auto big_label = stage_->ui->new_widget_as_label("Using a TrueType font!");
        big_label->resize(column, -1);
        big_label->set_font("Orbitron", 32);
        big_label->set_anchor_point(0, 1);
        big_label->move_to(x, y);

        if(added) {
            window->vfs->remove_search_path("simulant/fonts/orbitron");
        }

        auto simulant_logo = stage_->assets->new_texture_from_file("simulant/textures/simulant-icon.png");
        auto icon = stage_->ui->new_widget_as_image(simulant_logo);
        icon->set_anchor_point(1, 1);
        icon->move_to(window->coordinate_from_normalized(0.95, 0.95));

        //stage_->ui->transform_input_with_camera(camera_);
    }

    void update(float dt) {
        percent += (increasing) ? 100.0f * dt : -100.0f * dt;
        if(percent >= 100.0f) {
            increasing = false;
            percent = 100.0f;
        }

        if(percent <= 0.0f) {
            increasing = true;
            percent = 0.0f;
        }

        pg2_->set_value(percent);
        pg1_->pulse();
    }

private:
    smlt::StagePtr stage_;
    smlt::CameraPtr camera_;
    smlt::ui::ProgressBar* pg1_;
    smlt::ui::ProgressBar* pg2_;

    bool increasing = true;
    float percent = 0;
};

class App : public smlt::Application {
public:
    App(const smlt::AppConfig& config):
        smlt::Application(config) {

    }

    bool init() {
        scenes->register_scene<MainScene>("demo");
        scenes->register_scene<smlt::scenes::Splash>("main", "demo");

        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "UI Demo";
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
