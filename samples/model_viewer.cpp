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
    light.set_direction(1, 0, 0);
    //light.move_to(50, 0, 0);
    light.set_diffuse(kglt::Colour::yellow);

    kglt::Mesh& mesh = subscene.mesh(subscene.new_mesh());
    window->loader_for(filename)->into(mesh);

    kglt::Entity& entity = subscene.entity(subscene.new_entity(mesh.id()));
    entity.move_to(0, 0, -50);

    while(window->update()) {
        entity.rotate_y(10.0 * window->delta_time());

    }

    return 0;
}

