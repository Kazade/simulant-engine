#include "kglt/kglt.h"

#include "kglt/kazbase/string.h"

int main(int argc, char* argv[]) {        
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    if(argc < 2) {
        std::cout << "USAGE: model_viewer filename" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    kglt::Window::ptr window = kglt::Window::create(1024, 768);
    window->set_logging_level(kglt::LOG_LEVEL_DEBUG);
    window->set_title("KGLT Model viewer");

    kglt::SubScene& subscene = window->scene().subscene();
    subscene.set_ambient_light(kglt::Colour(1.0, 1.0, 1.0, 1.0));
    subscene.camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        1.0,
        1000.0
    );

    kglt::Light& light = subscene.light(subscene.new_light());
    //light.set_direction(-1, 0, 0);
    light.set_diffuse(kglt::Colour::yellow);
    light.set_attenuation_from_range(100.0);

    kglt::Light& light2 = subscene.light(subscene.new_light());
    light2.set_diffuse(kglt::Colour::red);
    light2.set_attenuation_from_range(100.0);
    light2.set_direction(0, 1, 0);

    //light.move_to(50, 0, -50);
    kglt::Mesh& mesh = subscene.mesh(subscene.new_mesh());
    window->loader_for(filename)->into(mesh);
    subscene.set_ambient_light(kglt::Colour(0.2, 0.2, 0.2, 0.2));
    //mesh.enable_debug(true);

    kglt::Entity& entity = subscene.entity(subscene.new_entity(mesh.id()));
    entity.move_to(0, 0, -50);

    float x_position = 0.0f;
    bool incrementing = true;

    while(window->update()) {
        entity.rotate_y(10.0 * window->delta_time());

        x_position += ((incrementing) ? -10.0 : 10.0) * window->delta_time();

        if(x_position < -100) {
            x_position += 0.1;
            incrementing = !incrementing;
        }
        if(x_position > 100) {
            x_position -= 0.1;
            incrementing = !incrementing;
        }

        light.move_to(x_position, 20.0, -50.0);
    }

    return 0;
}

