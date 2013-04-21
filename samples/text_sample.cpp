#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Text Sample");

    kglt::ui::Interface& ui = window->ui();
    ui.set_styles("body { font-family: \"Liberation Sans\"; } .thing { color: red; };");
    ui.append("<p>").text("Hello world!");
    ui._("p").add_class("thing");

    while(window->update()) {}

    return 0;
}

