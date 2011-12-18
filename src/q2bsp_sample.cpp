#include "GL/window.h"
#include "GL/types.h"
#include "GL/shortcuts.h"

GL::Window window(1024, 768);
std::vector<uint8_t> keys(SDLK_LAST, 0);

void on_key_down(SDL_keysym sym) {
    keys[sym.sym] = 1;
}

void on_key_up(SDL_keysym sym) {
    keys[sym.sym] = 0;
}

int main(int argc, char* argv[]) {
	window.set_title("KGLT Sample");

    window.scene().render_options.wireframe_enabled = false;
    window.scene().render_options.texture_enabled = true;
    window.scene().render_options.backface_culling_enabled = true;
    window.scene().render_options.point_size = 1;

    window.loader_for("sample.bsp")->into(window.scene());

    window.signal_key_pressed().connect(&on_key_down);
    window.signal_key_released().connect(&on_key_up);

	while(window.update()) {
        if(keys[SDLK_LEFT]) {
            window.scene().camera().rotate_y(-2.0f);
        }

        if(keys[SDLK_RIGHT]) {
            window.scene().camera().rotate_y(2.0f);
        }

        if(keys[SDLK_UP]) {
            window.scene().camera().move_forward(2.0f);
        }

	}
	return 0;
}
