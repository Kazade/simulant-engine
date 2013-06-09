#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));
    
    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Sample");

    kglt::Stage& stage = window->scene().stage();

    /**
        Generate a mesh and build a 2D square

        Base objects are always created with new_X() and can
        be destroyed with delete_X(). They are held by the object
        that spawned them. For example, meshes are held by the scene.

        Creating an object gives you an ID, this can then be exchanged
        for a reference to an object.
    */

    ///Shortcut function for loading images
    kglt::TextureID tid = stage.new_texture();
    auto tex = stage.texture(tid);
    kglt::procedural::texture::starfield(tex.__object);

    kglt::Actor& actor = stage.actor(stage.geom_factory().new_rectangle(2.0, 2.0));
    actor.mesh().lock()->set_texture_on_material(0, tid);

    kglt::MeshPtr mesh = actor.mesh().lock();
    kglt::MaterialID matid = mesh->submesh(mesh->submesh_ids()[0]).material_id();

    /**
        Once we have the reference to a base object, we can
        manipulate it easily
    */
    actor.move_to(0.0f, 0.0f, -2.0f);

    window->scene().camera().set_orthographic_projection_from_height(2.0, (float) window->width() / (float)window->height());

    while(window->update()) {
      //  stage.material(matid).lock()->technique().pass(0).texture_unit(0).scroll_x(0.5 * window->delta_time());
    }
	return 0;
}
