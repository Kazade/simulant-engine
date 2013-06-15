#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Text Sample");

    auto ui = window->scene().ui_stage();
    ui->set_styles("body { font-family: \"Ubuntu\"; } .thing { color: red; };");
    ui->append("<p>").text("Hello world!");
    ui->$("p").add_class("thing");

    while(window->update()) {}

    return 0;
}

