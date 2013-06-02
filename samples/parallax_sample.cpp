#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"

using kglt::extra::Background;

int main(int argc, char* argv[]) {
	//Set up logging to stdio
	logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();    
    kglt::Scene& scene = window->scene();
    kglt::Stage& stage = scene.stage();

    window->set_title("KGLT Parallax Sample");

	//Automatically calculate an orthographic projection, taking into account the aspect ratio
	//and the passed height. For example, passing a height of 2.0 would mean the view would extend
	//+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
//	window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    window->scene().camera().set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);
	
    //Create a background and add 3 layers to it
    Background::ptr background = Background::create(scene);
    background->add_layer("sample_data/parallax/back_layer.png");
    background->add_layer("sample_data/parallax/middle_layer.png");
    background->add_layer("sample_data/parallax/front_layer.png");

    while(window->update()) {
        background->layer(0).scroll_x(0.1 * window->delta_time());
        background->layer(1).scroll_x(0.2 * window->delta_time());
        background->layer(2).scroll_x(1.0 * window->delta_time());
	}

	return 0;
}
