#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
    
    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Sample");

    kglt::Stage& stage = window->scene().stage();
    window->scene().camera().set_perspective_projection(
        45.0,
        float(window->width()) / float(window->height()),
        1.0,
        1000.0
    );

    /**
        Generate a mesh and build a 2D square

        Base objects are always created with new_X() and can
        be destroyed with delete_X(). They are held by the object
        that spawned them. For example, meshes are held by the scene.

        Creating an object gives you an ID, this can then be exchanged
        for a reference to an object.
    */

    ///Shortcut function for loading images
    kglt::TextureID tid = kglt::create_texture_from_file(stage, "sample_data/sample.tga");
    kglt::MaterialID matid = kglt::create_material_from_texture(stage, tid);

    stage.set_ambient_light(kglt::Colour::white);

    kglt::Actor& entity = stage.entity(stage.geom_factory().new_capsule());
    entity.mesh().lock()->set_material_id(matid);

    /**
        Once we have the reference to a base object, we can
        manipulate it easily
    */
    entity.move_to(0.0f, 0.0f, -5.0f);

    //Set the entity to rotate each step
    window->signal_step().connect([&](float dt) { entity.rotate_y(20.0 * dt); });

    while(window->update()) {}

	return 0;
}
