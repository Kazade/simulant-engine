#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window window;
    window.set_title("KGLT Text Sample");

    //Automatically calculate an orthographic projection, taking into account the aspect ratio
    //and the passed height. For example, passing a height of 2.0 would mean the view would extend
    //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
    window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    window.scene().pass().renderer().set_orthographic_projection_from_height(480, 16.0 / 9.0);

    kglt::FontID fid = window.scene().new_font();
    kglt::Font& font = window.scene().font(fid);
    font.initialize("sample_data/sample.ttf", 18);

    kglt::Text& text = kglt::return_new_text(window.scene());
    text.set_text("Hello World!");
    text.apply_font(fid);

    while(window.update()) {}

    return 0;
}

