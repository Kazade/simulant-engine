#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();

    kglt::Scene& scene = window->scene();
    kglt::Stage& stage = scene.stage();

    {
        auto actor = stage.actor(stage.geom_factory().new_rectangle_outline(1.0, 1.0));
        actor->move_to(0.0, 0.0, -5.0);
    }
	
    while(window->update()) {}
	
	return 0;
}
