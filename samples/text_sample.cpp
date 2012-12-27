#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

#include "kglt/extra/ui/interface.h"

using namespace kglt::extra;

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();

    window->set_title("KGLT Text Sample");

    ui::Interface::ptr interface = ui::Interface::create(scene, 800, 600);
    interface->load_font("sample_data/sample.ttf", 12);

    ui::LabelID l = interface->new_label();
    interface->label(l).set_text("The quick brown fox jumps over the lazy dog?!$£%^&*(");
    interface->label(l).set_position(ui::Ratio(0.1), ui::Ratio(0.8));
    interface->label(l).set_size(ui::Ratio(0.4), ui::Ratio(0.1));
    interface->label(l).set_foreground_colour(kglt::Colour::red);

    while(window->update()) {}

    return 0;
}

