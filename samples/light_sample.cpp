#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();
    kglt::Stage& stage = scene.stage();

    window->set_title("Lighting Sample");
    stage.camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        0.1,
        1000.0
    );

    stage.set_ambient_light(kglt::Colour(0.2, 0.2, 0.2, 1.0));

    kglt::Entity& entity = stage.entity(scene.geom_factory().new_cube(stage.id(), kglt::Vec3(), 2.0));
    entity.move_to(0.0, 0.0, -5.0);

    kglt::Light& light = stage.light(stage.new_light());
    light.move_to(5.0, 0.0, -5.0);
    light.set_diffuse(kglt::Colour::green);
    light.set_attenuation_from_range(10.0);

    kglt::Light& light2 = stage.light(stage.new_light());
    light2.move_to(-5.0, 0.0, -5.0);
    light2.set_diffuse(kglt::Colour::blue);
    light2.set_attenuation_from_range(50.0);

    kglt::Light& light3 = stage.light(stage.new_light());
    light3.move_to(0.0, 15.0, -5.0);
    light3.set_diffuse(kglt::Colour::red);
    light3.set_attenuation_from_range(50.0);

    while(window->update()) {
        entity.rotate_x(window->delta_time() * 20.0);
        entity.rotate_y(window->delta_time() * 15.0);
        entity.rotate_z(window->delta_time() * 25.0);
    }

    return 0;
}
