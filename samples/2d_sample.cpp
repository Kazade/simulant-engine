#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/extra.h"

using namespace kglt::extra;

class GameScreen : public kglt::Screen<GameScreen> {
public:
    GameScreen(kglt::WindowBase& window):
        kglt::Screen<GameScreen>(window) {}

    void do_load() {
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
    }

    void do_activate() {
        window().enable_virtual_joypad(kglt::VIRTUAL_DPAD_DIRECTIONS_TWO, 2);
        window().message_bar().inform("Sample demonstrating 2D sprites");
    }
};


class Sample2D: public kglt::Application {
private:
    bool do_init() {
        register_screen("/", kglt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    Sample2D app;
    return app.run();
}
