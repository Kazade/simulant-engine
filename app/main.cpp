#include <kglt/kglt.h>

int main(int argc, char* argv[]) {
    /*
     *  FIXME: parse options
     */

    if(argc < 2) {
        std::cout << "USAGE: kglt lua_script [OPTIONS]" << std::endl;
        return 1;
    }

    std::string lua_file = std::string(argv[1]);
    kglt::Window::ptr window = kglt::Window::create();
    window->interpreter().run_file(lua_file);

    bool result = window->interpreter().call_function<bool>("kglt_init");
    if(!result) {
        std::cerr << "Unable to initialize the application" << std::endl;
        return 2;
    }

    while(window->update()) {}

    return 0;
}
