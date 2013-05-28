#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"

int main(int argc, char* argv[]) {
	//Set up logging to stdio
	logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    kglt::SubScene& subscene = window->scene().subscene();

    window->set_title("KGLT Sprite Sample");

    //Load the strip of sprites into separate textures
    kglt::extra::SpriteStripLoader loader(subscene, "sample_data/sonic.png", 64);
    std::vector<kglt::TextureID> frames = loader.load_frames();

    //Construct a Sprite object that takes care of handling materials, meshes etc.
    kglt::extra::Sprite::ptr sprite = kglt::extra::Sprite::create(window->scene().subscene_ref(subscene.id()));
    sprite->add_animation("stand", container::slice(frames, 4, 6), 0.5);
    sprite->set_render_dimensions(1.5, 1.5);
    sprite->move_to(0.0, 0.0, -1.0);

	//Automatically calculate an orthographic projection, taking into account the aspect ratio
	//and the passed height. For example, passing a height of 2.0 would mean the view would extend
	//+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
    //window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    subscene.camera().set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);
	
    while(window->update()) {}

	return 0;
}
