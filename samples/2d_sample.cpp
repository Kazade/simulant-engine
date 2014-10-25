#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/extra.h"

using namespace kglt::extra;

class Sample2D: public kglt::Application {
public:
    Sample2D():
        Application("KGLT Sprite Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {

        //Automatically calculate an orthographic projection, taking into account the aspect ratio
        //and the passed height. For example, passing a height of 2.0 would mean the view would extend
        //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
        float render_height = 16.0;
        float render_width = window().camera()->set_orthographic_projection_from_height(render_height, float(window().width()) / float(window().height()));

        //Load a sprite grid, from the 'Layer 1' layer in a tmx file
        kglt::MeshID mesh_id = window().stage()->new_mesh_from_tmx_file("sample_data/tiled/example.tmx", "Layer 1");
        window().stage()->new_actor_with_mesh(mesh_id);

        auto bounds = window().stage()->mesh(mesh_id)->aabb();

        //Constrain the camera to the area where the sprite grid is rendered
        window().stage()->camera()->constrain_to(
            kglt::Vec3(render_width / 2, render_height / 2, 0),
            kglt::Vec3(
                bounds.width() - render_width / 2,
                bounds.height() - render_height / 2,
                0
            )
        );

        window().enable_virtual_gamepad(kglt::VIRTUAL_DPAD_DIRECTIONS_TWO, 2);

        return true;
    }

    void do_step(double dt) {
        static bool shown = false;
        if(initialized() && !shown) {
            shown = true;
            window().message_bar().inform("Sample demonstrating 2D sprites");
        }
    }
    void do_cleanup() {
    }
};


int main(int argc, char* argv[]) {
    Sample2D app;
    return app.run();
}
