#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Text Sample");

    window->ui().set_styles("body { font-family: \"Liberation Sans\"; } .thing { color: red; };");
    window->ui().append("<p>").text("Hello world!");

    window->ui()._("p")[0].add_class("thing");

    while(window->update()) {}

    return 0;
}

