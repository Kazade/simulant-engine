


#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
	//Set up logging to stdio
	logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Parallax Sample");

    kglt::Scene& scene = window->scene();

	//Automatically calculate an orthographic projection, taking into account the aspect ratio
	//and the passed height. For example, passing a height of 2.0 would mean the view would extend
	//+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
//	window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    scene.camera().set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);
	
	//Alternatively window.scene().background().add_layer("sample_data/parallax/back_layer.png", BACKGROUND_FILL);
    scene.background().add_layer("sample_data/parallax/back_layer.png");
    scene.background().add_layer("sample_data/parallax/middle_layer.png");
    scene.background().add_layer("sample_data/parallax/front_layer.png");

    double width = scene.background().layer(0).width();
    double height = width / (16.0 / 9.0);
    scene.background().set_visible_dimensions(width, height); //The visible height in pixels (ortho)

    while(window->update()) {
        scene.background().layer(0).scroll_x(0.1 * window->delta_time());
        scene.background().layer(1).scroll_x(0.2 * window->delta_time());
        scene.background().layer(2).scroll_x(1.0 * window->delta_time());
	}

	return 0;
}
