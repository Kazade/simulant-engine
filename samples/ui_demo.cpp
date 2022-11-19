
#include <simulant/simulant.h>

class MainScene:
    public smlt::Scene<MainScene> {

public:
    MainScene(smlt::Window* window):
        smlt::Scene<MainScene>(window) {}

    void load() {
        stage_ = new_stage();
        camera_ = stage_->new_camera_with_orthographic_projection(0, window->width(), 0, window->height());

        auto pipeline = compositor->render(
            stage_, camera_
        )->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        pipeline->viewport->set_colour(smlt::Colour::GREY);

        link_pipeline(pipeline);

        auto coord = window->coordinate_from_normalized(0.05, 0.95);
        int x = coord.x;
        int y = coord.y;
        smlt::ui::Px spacing = 10;
        int column = window->coordinate_from_normalized(0.25, 0).x;

        auto frame = stage_->ui->new_widget_as_frame("UI Demo");
        frame->set_anchor_point(0.0f, 1.0f);
        frame->move_to(x, y);
        frame->set_padding(spacing);
        frame->set_space_between(spacing);

        auto label = stage_->ui->new_widget_as_label("Label");
        label->resize(column, -1);
        label->set_background_colour(smlt::ui::UIConfig().foreground_colour_);
        frame->pack_child(label);

        auto button = stage_->ui->new_widget_as_button("Button");
        button->resize(column, -1);
        frame->pack_child(button);

        pg1_ = stage_->ui->new_widget_as_progress_bar();
        pg1_->set_text("Progress Bar (pulse)");
        pg1_->resize(column, -1);
        pg1_->set_opacity(0.2f);
        frame->pack_child(pg1_);

        pg2_ = stage_->ui->new_widget_as_progress_bar();
        pg2_->set_text("Progress Bar (percent)");
        pg2_->resize(column, -1);
        frame->pack_child(pg2_);

        auto big_label = stage_->ui->new_widget_as_label("Using a TrueType font!");
        big_label->resize(column, -1);
        big_label->set_font("Orbitron", 32);
        frame->pack_child(big_label);

        auto simulant_logo = stage_->assets->new_texture_from_file("textures/simulant-icon.png");
        auto icon = stage_->ui->new_widget_as_image(simulant_logo);
        icon->set_anchor_point(1, 1);
        icon->move_to(window->coordinate_from_normalized(0.95, 0.95));

        //stage_->ui->transform_input_with_camera(camera_);

        auto fixed_width = stage_->ui->new_widget_as_label("This is some text with a fixed width.\n See it works!");
        fixed_width->resize(200, -1);
        fixed_width->move_to(100, 200);
        fixed_width->set_background_colour(smlt::Colour::PURPLE);
        fixed_width->set_border_radius(smlt::ui::Px(10));
        fixed_width->set_padding(10);

        auto fixed_height = stage_->ui->new_widget_as_label("This is some text with a fixed height.\n See it works!");
        fixed_height->resize(-1, 200);
        fixed_height->move_to(300, 200);
        fixed_height->set_background_colour(smlt::Colour::PURPLE);

        auto fit_content = stage_->ui->new_widget_as_label("This widget fits its text content. See it works!");
        fit_content->resize(-1, -1);
        fit_content->move_to(700, 200);
        fit_content->set_background_colour(smlt::Colour::PURPLE);

        auto pl = stage_->ui->new_widget_as_label("PL");
        pl->set_padding(10, 0, 0, 0);
        //pl->resize(80, -1);
        pl->set_background_colour(smlt::Colour::GREY);
        pl->set_anchor_point(1.0f, 1.0f);
        pl->move_to(window->coordinate_from_normalized(0.75f, 0.75f));


        auto left_label = stage_->ui->new_widget_as_label("This label has left alignment");
        auto middle_label = stage_->ui->new_widget_as_label("This label has center alignment");
        auto right_label = stage_->ui->new_widget_as_label("This label has right alignment");

        left_label->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_LEFT);
        middle_label->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
        right_label->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_RIGHT);

        auto max = std::max(std::max(left_label->content_width(), right_label->content_width()), middle_label->content_width());
        left_label->resize(max + 128, -1);
        middle_label->resize(max + 128, -1);
        right_label->resize(max + 128, -1);

        auto align_frame = stage_->ui->new_widget_as_frame("");
        align_frame->pack_child(left_label);
        align_frame->pack_child(middle_label);
        align_frame->pack_child(right_label);
        align_frame->set_anchor_point(1.0f, 1.0f);
        align_frame->move_to(window->width() - 16, window->height() - 16);
    }

    void activate() override {
        auto entry = stage_->ui->new_widget_as_label("");

        if(!input->start_text_input(false)) {
            /* No on-screen keyboard, so show a dialog */
            auto dialog = stage_->ui->new_widget_as_frame("Please enter some text");
            dialog->pack_child(entry);
            dialog->move_to(window->width() / 2, window->height() / 2);
        } else {
            entry->move_to(window->width() / 2, window->height() / 2);
        }

        input->signal_text_input_received().connect([=](const char16_t& chr, smlt::TextInputReceivedControl&) -> bool {
            auto txt = entry->text();
            txt.push_back(chr);
            entry->set_text(txt);
            return true;
        });
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
        scenes->register_scene<MainScene>("main");

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
