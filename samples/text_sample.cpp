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

    while(window->update()) {}

    return 0;
}

