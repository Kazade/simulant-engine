#include "kglt/kglt.h"
#include "kglt/shortcuts.h"


std::vector<uint8_t> keys(kglt::KEY_CODE_LAST, 0);

void on_key_down(kglt::KeyCode sym) {
    keys[sym] = 1;
}

void on_key_up(kglt::KeyCode sym) {
    keys[sym] = 0;
}

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create(1024, 768);
    window->set_title("Quake 2 Renderer");

    kglt::SubScene& subscene = window->scene().subscene();
    subscene.set_ambient_light(kglt::Colour(0.02, 0.02, 0.02, 1.0));

    subscene.camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        0.1,
        1000.0
    );

    //Create a shader
    //Shader& shader = window.scene().shader(window.scene().new_shader());

    //Load the lighting shader
    //window.loader_for("lighting.shader")->into(shader);

    //Load the Quake 2 map
    window->loader_for("sample_data/sample.bsp")->into(window->scene());

    kglt::Keyboard& keyboard = window->keyboard();
    keyboard.key_while_down_connect(kglt::KEY_CODE_LEFT, std::bind(&kglt::Camera::rotate_y, &subscene.camera(), -4.0));
    keyboard.key_while_down_connect(kglt::KEY_CODE_RIGHT, std::bind(&kglt::Camera::rotate_y, &subscene.camera(), 4.0));
    keyboard.key_while_down_connect(kglt::KEY_CODE_UP, std::bind(&kglt::Camera::move_forward, &subscene.camera(), 10.0));
    keyboard.key_while_down_connect(kglt::KEY_CODE_DOWN, std::bind(&kglt::Camera::move_forward, &subscene.camera(), -10.0));

    while(window->update()) {}

    return 0;
}
