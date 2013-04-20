#include "kglt/kglt.h"

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();

    window->set_title("KGLT UI Sample");
    window->load_ui("sample_data/demo.rml");

/*
    interface->load_font("sample_data/sample.ttf", 12);

    ui::Label& label = interface->label(interface->new_label());
    label.set_text(u8"The quick brown fox jumped over the lazy dog \u00a9");
    label.set_position(ui::Ratio(0.01), ui::Ratio(0.2));
    label.set_foreground_colour(kglt::Colour::black);
    label.set_size(ui::Ratio(0.5), ui::Ratio(0.2));
*/
    while(window->update()) {}

    return 0;
}
