
#include <simulant/simulant.h>

class MainScreen:
    public smlt::Screen<MainScreen> {

public:
    MainScreen(smlt::WindowBase& window):
        smlt::Screen<MainScreen>(window, "UI Demo") {}

    void do_load() {
        camera_ = window->new_camera_with_orthographic_projection(0, window->width(), 0, window->height());
        stage_ = window->new_stage();
        window->render(stage_, camera_).with_clear();

        smlt::StagePtr stage = stage_.fetch();

        //stage->ui->transform_input_with_camera(camera_);

        auto title = stage->ui->new_widget_as_label("UI Sample demonstrating widgets").fetch();
        title->move_to(window->coordinate_from_normalized(0.5, 0.9));

        auto button = stage->ui->new_widget_as_button("Button 1").fetch();
        button->move_to(window->coordinate_from_normalized(0.1, 0.25));
    }

private:
    smlt::StageID stage_;
    smlt::CameraID camera_;
};

class App : public smlt::Application {
public:
    bool do_init() {
        register_screen("/", smlt::screen_factory<MainScreen>());
        return true;
    }
};

int main(int argc, char* argv[]) {
    App app;
    return app.run();
}
