#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();

    auto stage = window->stage();

    {
        auto actor = stage->actor(stage->geom_factory().new_rectangle_outline(1.0, 1.0));
        actor->move_to(0.0, 0.0, -5.0);
    }
	
    while(window->run_frame()) {}
	
	return 0;
}
