
#include <simulant/simulant.h>

class MainScene: public smlt::Scene {

public:
    MainScene(smlt::Window* window):
        smlt::Scene(window) {}

    void load() {
        camera_ = create_node<smlt::Camera>();
        camera_->set_orthographic_projection(0, window->width(), 0, window->height());

        auto pipeline = compositor->render(
            this, camera_
        )->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        pipeline->viewport->set_colour(smlt::Colour::GREY);

        link_pipeline(pipeline);

        auto coord = window->coordinate_from_normalized(0.05, 0.95);
        int x = coord.x;
        int y = coord.y;
        smlt::ui::Px spacing = 10;
        int column = window->coordinate_from_normalized(0.25, 0).x;

        auto frame = create_node<smlt::ui::Frame>("UI Demo");
        frame->set_anchor_point(0.0f, 1.0f);
        frame->move_to(x, y);
        frame->set_padding(spacing);
        frame->set_space_between(spacing);

        auto label = create_node<smlt::ui::Label>("Label");
        label->resize(column, -1);
        label->set_background_colour(smlt::ui::UIConfig().foreground_colour_);
        frame->pack_child(label);

        auto button = create_node<smlt::ui::Button>("Button");
        button->resize(column, -1);
        frame->pack_child(button);

        pg1_ = create_node<smlt::ui::ProgressBar>();
        pg1_->set_text("Progress Bar (pulse)");
        pg1_->resize(column, -1);
        pg1_->set_opacity(0.2f);
        frame->pack_child(pg1_);

        pg2_ = create_node<smlt::ui::ProgressBar>();
        pg2_->set_text("Progress Bar (percent)");
        pg2_->resize(column, -1);
        frame->pack_child(pg2_);

        smlt::FontFlags flags;
        flags.blur_radius = 1;
        flags.size = 32;
        auto blurred_font = assets->new_font_from_file("fonts/Orbitron/Orbitron-Regular.ttf", flags);

        auto big_label = create_node<smlt::ui::Label>("Using a TrueType font!");
        big_label->resize(column, -1);
        big_label->set_font(blurred_font);
        frame->pack_child(big_label);

        auto simulant_logo = assets->new_texture_from_file("textures/simulant-icon.png");
        auto icon = create_node<smlt::ui::Image>(simulant_logo);
        icon->set_anchor_point(1, 1);
        icon->move_to(window->coordinate_from_normalized(0.95, 0.95));

        //stage_->ui->transform_input_with_camera(camera_);

        auto fixed_width = create_node<smlt::ui::Label>("This is some long text with a fixed width.\n See it works!");
        fixed_width->resize(200, -1);
        fixed_width->move_to(400, 500);
        fixed_width->set_background_colour(smlt::Colour::PURPLE);
        fixed_width->set_border_radius(smlt::ui::Px(10));
        fixed_width->set_padding(10);

        auto fixed_height = create_node<smlt::ui::Label>("This is some text with a fixed height.\n See it works!");
        fixed_height->resize(-1, 200);
        fixed_height->move_to(300, 200);
        fixed_height->set_background_colour(smlt::Colour::PURPLE);

        auto fit_content = create_node<smlt::ui::Label>("This widget fits its text content. See it works!");
        fit_content->resize(-1, -1);
        fit_content->move_to(700, 200);
        fit_content->set_background_colour(smlt::Colour::PURPLE);

        auto pl = create_node<smlt::ui::Label>("PL");
        pl->set_padding(10, 0, 0, 0);
        //pl->resize(80, -1);
        pl->set_background_colour(smlt::Colour::GREY);
        pl->set_anchor_point(1.0f, 1.0f);
        pl->move_to(window->coordinate_from_normalized(0.75f, 0.75f));


        auto left_label = create_node<smlt::ui::Label>("This label has left alignment");
        auto middle_label = create_node<smlt::ui::Label>("This label has center alignment");
        auto right_label = create_node<smlt::ui::Label>("This label has right alignment");

        left_label->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_LEFT);
        middle_label->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_CENTER);
        right_label->set_text_alignment(smlt::ui::TEXT_ALIGNMENT_RIGHT);

        auto max = std::max(std::max(left_label->content_width(), right_label->content_width()), middle_label->content_width());
        left_label->resize(max + 128, -1);
        middle_label->resize(max + 128, -1);
        right_label->resize(max + 128, -1);

        auto align_frame = create_node<smlt::ui::Frame>("");
        align_frame->pack_child(left_label);
        align_frame->pack_child(middle_label);
        align_frame->pack_child(right_label);
        align_frame->set_anchor_point(1.0f, 1.0f);
        align_frame->move_to(window->width() - 16, window->height() - 16);
    }

    void activate() override {
        auto entry = create_node<smlt::ui::Label>("");

        if(!input->start_text_input(true)) {
            /* No on-screen keyboard, so show a dialog */
            auto dialog = create_node<smlt::ui::Frame>("Please enter some text");
            dialog->pack_child(entry);
            dialog->move_to(window->width() / 2, window->height() / 2);
        } else {
            entry->move_to(window->width() / 2, window->height() / 2);
        }

        input->signal_text_input_received().connect([=](const unicode& chr, smlt::TextInputEvent&) -> bool {
            auto txt = entry->text();
            entry->set_text(txt + chr);
            return true;
        });
    }

    void on_update(float dt) {
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
