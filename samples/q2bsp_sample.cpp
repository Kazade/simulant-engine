#include "kglt/window.h"
#include "kglt/types.h"

kglt::Window window(1024, 768);
std::vector<uint8_t> keys(kglt::KEY_CODE_LAST, 0);

void on_key_down(kglt::KeyCode sym) {
    keys[sym] = 1;
}

void on_key_up(kglt::KeyCode sym) {
    keys[sym] = 0;
}

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
	window.set_title("KGLT Sample");

    window.scene().render_options.wireframe_enabled = false;
    window.scene().render_options.texture_enabled = true;
    window.scene().render_options.backface_culling_enabled = false;
    window.scene().render_options.point_size = 1;

    window.scene().active_camera().set_perspective_projection(
        45.0,
        float(window.width()) / float(window.height()),
        0.1,
        1000.0
    );

    //Create a shader
    //Shader& shader = window.scene().shader(window.scene().new_shader());

    //Load the lighting shader
    //window.loader_for("lighting.shader")->into(shader);

    //Load the Quake 2 map
    window.loader_for("sample_data/sample.bsp")->into(window.scene());

    window.signal_key_down().connect(&on_key_down);
    window.signal_key_up().connect(&on_key_up);

	while(window.update()) {
        if(keys[kglt::KEY_CODE_LEFT]) {
            window.scene().camera().rotate_y(-4.0 * window.delta_time());
        }

        if(keys[kglt::KEY_CODE_RIGHT]) {
            window.scene().camera().rotate_y(4.0 * window.delta_time());
        }

        if(keys[kglt::KEY_CODE_UP]) {
            window.scene().camera().move_forward(15.0 * window.delta_time());
        }
        if(keys[kglt::KEY_CODE_DOWN]) {
            window.scene().camera().move_forward(-15.0 * window.delta_time());
        }

	}
	return 0;
}
