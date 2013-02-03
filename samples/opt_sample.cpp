#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/kazbase/string.h"
#include "kglt/extra/skybox.h"

int main(int argc, char* argv[]) {        
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    if(argc < 2) {
        std::cout << "USAGE: optview filename" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    if(!str::contains(str::lower(filename), ".opt")) {
        std::cout << "Please specify a file with a .opt extension" << std::endl;
        return 2;
    }

    kglt::Window::ptr window = kglt::Window::create(1024, 768);
    window->set_title("OPT Renderer");

    kglt::SubScene& subscene = window->scene().subscene();
    subscene.set_ambient_light(kglt::Colour(1.0, 1.0, 1.0, 1.0));
    subscene.camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        10.0,
        10000.0
    );

    kglt::Mesh& mesh = subscene.mesh(subscene.new_mesh());
    window->loader_for(filename)->into(mesh);

    kglt::Entity& entity = subscene.entity(subscene.new_entity(mesh.id()));
    entity.move_to(0, 0, 50);

    //Create a sky sphere
    kglt::Mesh& mesh2 = subscene.mesh(subscene.new_mesh());
    kglt::procedural::mesh::cube(mesh2, 300.0);

    ///Shortcut function for loading images
    kglt::TextureID tid = subscene.new_texture();
    kglt::procedural::texture::starfield(subscene.texture(tid));

    kglt::extra::SkyBox::ptr sky = kglt::extra::SkyBox::create(subscene, tid);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    while(window->update()) {
        entity.rotate_y(10.0 * window->delta_time());
    }

    return 0;
}

