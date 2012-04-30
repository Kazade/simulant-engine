#include "kglt/window.h"
#include "kglt/types.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

	kglt::Window window;
	window.set_title("KGLT Sprite Sample");

    kglt::SpriteID sprite_id = window.scene().new_sprite();
    kglt::Sprite& sprite = window.scene().sprite(sprite_id);

    //Get a sprite loader, set the sprite frame-with option to 40 pixels
    window.loader_for("sample_data/sonic.png", "LOADER_HINT_SPRITE")->into(sprite, { "SPRITE_FRAME_WIDTH", "40" });
    sprite.set_render_dimensions(0.5, 1.0); //Render the sprite 0.5 x 1.0

    //Automatically calculate an orthographic projection, taking into account the aspect ratio
    //and the passed height. For example, passing a height of 2.0 would mean the view would extend
    //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
    window.scene().viewport().set_orthographic_projection_from_height((float) 224 / (float) 40);

    while(window.update()) {}

    return 0;
}
