#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();
		
	kglt::ViewportID first = window->new_viewport();
    kglt::ViewportID second = window->new_viewport();

    window->viewport(first)->configure(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT);
    window->viewport(second)->configure(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT);

    window->viewport(first)->set_background_colour(kglt::Colour(1.0, 0, 0, 0));
    window->viewport(second)->set_background_colour(kglt::Colour(0, 1.0, 0, 0));

    window->render(window->new_stage(), window->new_camera()).to_framebuffer(first);
    window->render(window->new_stage(), window->new_camera()).to_framebuffer(second);

    while(window->run_frame()) {}
	
	return 0;
}
