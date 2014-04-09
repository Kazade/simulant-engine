#include <kglt/kglt.h>

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();

    window->set_title("KGLT UI Sample");
    window->scene().ui_stage()->load_rml("sample_data/demo.rml");

    while(window->run_frame()) {}

    return 0;
}
