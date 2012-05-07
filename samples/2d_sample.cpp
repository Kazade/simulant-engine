#include "kglt/window.h"
#include "kglt/types.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

	kglt::Window window;
	window.set_title("KGLT Sprite Sample");

	window.scene().render_options.backface_culling_enabled = false;
    window.scene().render_options.texture_enabled = true;

	//Shortcut function to create and then return a new sprite
	kglt::Sprite& sprite = kglt::return_new_sprite(window.scene());

    //Get a sprite loader, set the sprite frame-with option to 40 pixels
    window.loader_for("sample_data/sonic.png", "LOADER_HINT_SPRITE")->into(sprite, { "SPRITE_FRAME_WIDTH", "64" });
    sprite.set_render_dimensions(1.5, 1.5); //Render the sprite 0.5 x 1.0
    sprite.move_to(0.0, 0.0, -1.0);
	sprite.set_animation_frames(4, 5);
	sprite.set_animation_fps(8.0);

    //Automatically calculate an orthographic projection, taking into account the aspect ratio
    //and the passed height. For example, passing a height of 2.0 would mean the view would extend
    //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
    window.scene().viewport().set_orthographic_projection_from_height((float) 224 / (float) 40);
	
    while(window.update()) {
		//window.scene().camera().rotate_y(1.0);
	}

    return 0;
}
