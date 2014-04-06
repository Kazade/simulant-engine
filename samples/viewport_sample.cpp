#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();
	
	//scene.pass().viewport().configure(VIEWPORT_16_BY_9);
	
    kglt::ViewportID first = window->viewport().id(); //Get the default viewport ID
    kglt::ViewportID second = window->new_viewport();

    window->viewport(first).configure(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT);
    window->viewport(second).configure(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT);

    window->viewport(first).set_background_colour(kglt::Colour(1.0, 0, 0, 0));
    window->viewport(second).set_background_colour(kglt::Colour(0, 1.0, 0, 0));

    //Add another pass to render to the second viewport
    scene.render_sequence().new_pipeline(scene.stage()->id(), kglt::CameraID(), second);
		
    while(window->update()) {}
	
	return 0;
}
