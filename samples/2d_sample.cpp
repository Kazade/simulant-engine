#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"

using namespace smlt;
using namespace smlt::extra;

class GameScene : public smlt::Scene<GameScene> {
public:
    GameScene(smlt::Window* window):
        smlt::Scene<GameScene>(window) {}

    void load() {
        prepare_basic_scene(stage_id_, camera_id_);

        auto cam = camera_id_.fetch();

        //Automatically calculate an orthographic projection, taking into account the aspect ratio
        //and the passed height. For example, passing a height of 2.0 would mean the view would extend
        //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
        float render_height = 16.0;
        float render_width = cam->set_orthographic_projection_from_height(
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

    void activate() {
        window->enable_virtual_joypad(smlt::VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS);
    }

private:
    StageID stage_id_;
    CameraID camera_id_;
};


class Sample2D: public smlt::Application {
public:
    Sample2D(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        scenes->load_in_background("main", true); //Do loading in a background thread, but show immediately when done
        scenes->activate("_loading"); // Show the loading screen in the meantime
        return true;
    }
};


int main(int argc, char* argv[]) {
    smlt::AppConfig config;
    config.title = "2D Sample";
    config.fullscreen = false;
    config.width = 1280;
    config.height = 960;

    Sample2D app(config);
    return app.run();
}
