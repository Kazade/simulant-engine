#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();

    kglt::Scene& scene = window->scene();
    kglt::Stage& stage = scene.stage();
    kglt::Entity& entity = stage.entity(stage.geom_factory().new_rectangle_outline(1.0, 1.0));

    entity.move_to(0.0, 0.0, -5.0);
	
    while(window->update()) {}
	
	return 0;
}
