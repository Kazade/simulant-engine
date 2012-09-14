#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window window;

    window.scene().render_options.backface_culling_enabled = false;
    window.scene().active_camera().set_perspective_projection(
        45.0,
        float(window.width()) / float(window.height()),
        0.1,
        1000.0
    );

    kglt::Scene& scene = window.scene();

    kglt::Mesh& mesh = kglt::return_new_mesh(scene);
    kglt::procedural::mesh::cube(mesh, 3.0);
    mesh.move_to(0.0, 0.0, -5.0);

    kglt::Light& light = kglt::return_new_light(scene);
    light.move_to(1.0, 0.0, 0.0);
    light.set_diffuse(kglt::Colour(1.0, 0.0, 0.0, 1.0));
    light.set_attenuation_from_range(100.0);

    kglt::Light& light2 = kglt::return_new_light(scene);
    light2.move_to(-1.0, 0.0, 0.0);
    light2.set_diffuse(kglt::Colour(0.0, 0.0, 1.0, 1.0));
    light2.set_attenuation_from_range(100.0);

    while(window.update()) {}

    return 0;
}
