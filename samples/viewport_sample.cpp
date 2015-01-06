#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    kglt::Window::ptr window = kglt::Window::create();
		
    // Create two viewports for the left and right hand side of the screen, set different clear colours
    kglt::Viewport first(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT, kglt::Colour::RED);
    kglt::Viewport second(kglt::VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT, kglt::Colour::GREEN);

    // Render new stages to the framebuffer, using both viewports. Make sure we tell the pipeline to clear
    window->render(window->new_stage(), window->new_camera()).to_framebuffer(first).with_clear();
    window->render(window->new_stage(), window->new_camera()).to_framebuffer(second).with_clear();

    while(window->run_frame()) {}
	
	return 0;
}
