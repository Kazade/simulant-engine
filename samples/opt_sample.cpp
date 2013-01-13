#include "kglt/kglt.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create(1024, 768);
    window->set_title("OPT Renderer");

    kglt::SubScene& subscene = window->scene().subscene();
    subscene.set_ambient_light(kglt::Colour(0.02, 0.02, 0.02, 1.0));

    subscene.camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        0.1,
        1000.0
    );

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    kglt::Mesh& mesh = subscene.mesh(subscene.new_mesh());
    window->loader_for("sample_data/t47/SNOWSPEEDER.OPT")->into(mesh);

    kglt::Entity& entity = subscene.entity(subscene.new_entity(mesh.id()));
    entity.move_to(0, 0, -500);

    while(window->update()) {}

    return 0;
}

