#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window window;
    window.set_title("KGLT Parallax Sample");

    //Automatically calculate an orthographic projection, taking into account the aspect ratio
    //and the passed height. For example, passing a height of 2.0 would mean the view would extend
    //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
    window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    window.scene().active_camera().set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);

    //Alternatively window.scene().background().add_layer("sample_data/parallax/back_layer.png", BACKGROUND_FILL);
    window.scene().background().add_layer("sample_data/parallax/back_layer.png");
    window.scene().background().add_layer("sample_data/parallax/middle_layer.png");
    window.scene().background().add_layer("sample_data/parallax/front_layer.png");

    double width = window.scene().background().layer(0).width();
    double height = width / (16.0 / 9.0);
    window.scene().background().set_visible_dimensions(width, height); //The visible height in pixels (ortho)

    //Shortcut function to create and then return a new sprite
    kglt::Sprite& sprite = kglt::return_new_sprite(window.scene());

    //Get a sprite loader, set the sprite frame-width option to 64 pixels
    window.loader_for("sample_data/sonic.png", "LOADER_HINT_SPRITE")->into(sprite, { "SPRITE_FRAME_WIDTH", "64" });
    sprite.set_render_dimensions(1.5, 1.5); //Render the sprite 1.5 x 1.5 units
    sprite.move_to(0.0, -2.0, -1.0);
    sprite.set_animation_frames(31, 34);
    sprite.set_animation_fps(8.0);


    //First, load a default font
    kglt::FontID fid = window.scene().new_font();
    kglt::Font& font = window.scene().font(fid);
    font.initialize("sample_data/sample.ttf", 12);

    //Set the default font for the UI
    kglt::UI& interface = window.scene().ui();
    interface.set_default_font_id(fid);

    //Create a label
    kglt::ui::LabelID label_id = window.scene().ui().new_label();
    kglt::ui::Label& label = window.scene().ui().label(label_id);

    label.set_foreground_colour(kglt::Colour(1.0, 1.0, 0.0, 1.0));
    label.set_text("Score: 8739204");
    label.set_position(0.02, 0.9);

    while(window.update()) {
        window.scene().background().layer(0).scroll_x(0.1 * window.delta_time());
        window.scene().background().layer(1).scroll_x(0.2 * window.delta_time());
        window.scene().background().layer(2).scroll_x(1.0 * window.delta_time());
    }

    return 0;
}

