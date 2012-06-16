#include "kglt/kglt.h"
#include "kglt/ui/shortcuts.h"

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window window;
    window.set_title("KGLT UI Sample");

    //Automatically calculate an orthographic projection, taking into account the aspect ratio
    //and the passed height. For example, passing a height of 2.0 would mean the view would extend
    //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
    window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    window.scene().pass().renderer().set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);

    //First, load a default font
    kglt::FontID fid = window.scene().new_font();
    kglt::Font& font = window.scene().font(fid);
    font.initialize("sample_data/sample.ttf", 18);

    //Set the default font for the UI
    kglt::UI& interface = window.scene().ui();
    interface.set_default_font_id(fid);

    kglt::ui::Label& label = kglt::ui::return_new_label(window.scene());
    label.set_text("Hello World!");
    label.set_position(0.2, 0.2); //All positions and heights are between 0.0 and 1.0 for resolution independence
    //label.set_font_size((1.0 / 480.0) * 12.0); //At 640x480, this will be equivalent to 12pt
/*
    kglt::ui::Button& button = kglt::ui::return_new_button(interface);
    button.set_text("<b>Click me!</b>");
    button.set_position(0.9, 0.9);*/

    while(window.update()) {}

    return 0;
}
