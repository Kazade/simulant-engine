#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/extra.h"

int main(int argc, char* argv[]) {
	//Set up logging to stdio
	logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();    
    window->set_title("KGLT Parallax Sample");

	//Automatically calculate an orthographic projection, taking into account the aspect ratio
	//and the passed height. For example, passing a height of 2.0 would mean the view would extend
	//+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
//	window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    window->camera()->set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);
	
    //Create a background and add 3 layers to it
    window->new_background_from_file("sample_data/parallax/back_layer.png", 0.1);
    window->new_background_from_file("sample_data/parallax/middle_layer.png", 0.2);
    window->new_background_from_file("sample_data/parallax/front_layer.png", 1.0);

    while(window->run_frame()) {

	}

	return 0;
}
