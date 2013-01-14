#include "kglt/kglt.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create(1024, 768);
    window->set_title("OPT Renderer");

    kglt::SubScene& subscene = window->scene().subscene();
    subscene.set_ambient_light(kglt::Colour(1.0, 1.0, 1.0, 1.0));
    subscene.camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        0.1,
        1000.0
    );

    kglt::Mesh& mesh = subscene.mesh(subscene.new_mesh());
    window->loader_for("/home/kazade/Desktop/FLIGHTMODELS/TIEFIGHTER.OPT")->into(mesh);


    kglt::Entity& entity = subscene.entity(subscene.new_entity(mesh.id()));
    entity.move_to(0, 0, -500);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    while(window->update()) {
        entity.rotate_y(10.0 * window->delta_time());

    }

    return 0;
}

