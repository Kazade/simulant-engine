#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"

using namespace smlt;
using namespace smlt::extra;

class GameScreen : public smlt::Screen<GameScreen> {
public:
    GameScreen(smlt::WindowBase& window):
        smlt::Screen<GameScreen>(window, "game_screen") {}

    void do_load() {
        prepare_basic_scene(stage_id_, camera_id_);

        //Automatically calculate an orthographic projection, taking into account the aspect ratio
        //and the passed height. For example, passing a height of 2.0 would mean the view would extend
        //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
        float render_height = 16.0;
        float render_width = window->camera(camera_id_)->set_orthographic_projection_from_height(
            render_height, float(window->width()) / float(window->height())
        );

        {
            auto stage = window->stage(stage_id_);

            //Load a sprite grid, from the 'Layer 1' layer in a tmx file
            smlt::MeshID mesh_id = stage->assets->new_mesh_from_tmx_file(
                "sample_data/tiled/example.tmx", "Layer 1"
            );

            stage->new_actor_with_mesh(mesh_id);

            auto bounds = stage->assets->mesh(mesh_id)->aabb();

            stage->host_camera(camera_id_);

            //Constrain the camera to the area where the sprite grid is rendered
            stage->camera(camera_id_)->constrain_to_aabb(
                AABB(
                    smlt::Vec3(render_width / 2, render_height / 2, 0),
                    smlt::Vec3(
                        bounds.width() - render_width / 2,
                        bounds.height() - render_height / 2,
                        0
                    )
                )
            );

        }
    }

    void do_activate() {
        window->enable_virtual_joypad(smlt::VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS);
    }

private:
    StageID stage_id_;
    CameraID camera_id_;
};


class Sample2D: public smlt::Application {
private:
    bool do_init() {
        register_screen("/", smlt::screen_factory<GameScreen>());
        load_screen_in_background("/", true); //Do loading in a background thread, but show immediately when done
        activate_screen("/loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    Sample2D app;
    return app.run();
}
