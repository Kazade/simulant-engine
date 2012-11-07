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

    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("Lighting Sample");
    window->scene().camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        0.1,
        1000.0
    );

    kglt::Scene& scene = window->scene();
    scene.set_ambient_light(kglt::Colour(0.2, 0.2, 0.2, 1.0));

    kglt::Mesh& mesh = kglt::return_new_mesh(scene);
    kglt::procedural::mesh::cube(mesh, 1.0);

    kglt::Entity& entity = scene.entity(scene.new_entity(mesh.id()));
    entity.move_to(0.0, 0.0, -5.0);

    kglt::Light& light = kglt::return_new_light(scene);
    light.move_to(1.0, 0.0, 0.0);
    light.set_diffuse(kglt::Colour(1.0, 0.0, 0.0, 1.0));
    light.set_attenuation_from_range(100.0);

    kglt::Light& light2 = kglt::return_new_light(scene);
    light2.move_to(-1.0, 0.0, 0.0);
    light2.set_diffuse(kglt::Colour(0.0, 0.0, 1.0, 1.0));
    light2.set_attenuation_from_range(100.0);

    window->signal_key_down().connect(&on_key_down);
    window->signal_key_up().connect(&on_key_up);

    while(window->update()) {
        if(keys[kglt::KEY_CODE_LEFT]) {
            scene.camera().rotate_y(-1.0 * window->delta_time());
        }

        if(keys[kglt::KEY_CODE_RIGHT]) {
            scene.camera().rotate_y(1.0 * window->delta_time());
        }

        if(keys[kglt::KEY_CODE_UP]) {
            scene.camera().move_forward(1.0 * window->delta_time());
        }
        if(keys[kglt::KEY_CODE_DOWN]) {
            scene.camera().move_forward(-1.0 * window->delta_time());
        }
    }

    return 0;
}
