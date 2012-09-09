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
    kglt::procedural::mesh::cube(mesh, 1.0);
    mesh.move_to(0.0, 0.0, -5.0);

    kglt::Light& light = kglt::return_new_light(scene);
    light.move_to(2.0, 20.0, 0.0);

    while(window.update()) {}

    return 0;
}
