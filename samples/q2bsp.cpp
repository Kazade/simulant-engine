#include "simulant/simulant.h"
#include "simulant/shortcuts.h"
#include "simulant/extra.h"

using namespace smlt;

class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window):
        smlt::Scene(window) {}

    void load() {
        camera_ = create_node<smlt::Camera>();
        pipeline_ = compositor->render(this, camera_);

        pipeline_->set_clear_flags(BUFFER_CLEAR_ALL);
        pipeline_->viewport->set_colour(smlt::Colour::GREY);
        link_pipeline(pipeline_);

        app->vfs->add_search_path("sample_data/quake2/textures");

        auto mesh = assets->new_mesh_from_file("sample_data/quake2/maps/demo1.bsp");
        create_node<smlt::Geom>(mesh);

        cr_yield();

        auto entities = mesh->data->get<smlt::Q2EntityList>("entities");

        std::for_each(entities.begin(), entities.end(), [&](Q2Entity& ent) {
            if(ent["classname"] == "info_player_start") {
                auto position = ent["origin"];
                std::vector<unicode> coords = _u(position).split(" ");

                //Needed because the Quake 2 coord system is weird
                Mat4 rotation_x = Mat4::as_rotation_x(Degrees(90));
                Mat4 rotation_y = Mat4::as_rotation_y(Degrees(90.0f));
                Mat4 rotation = rotation_y * rotation_x;

                smlt::Vec3 pos(
                    coords[0].to_float(),
                    coords[1].to_float(),
                    coords[2].to_float()
                );

                pos = pos.rotated_by(rotation);
                camera_->transform->set_position(pos);
            }

            cr_yield();
        });

        auto fly = create_node<smlt::FlyController>();
        camera_->set_parent(fly);

        camera_->set_perspective_projection(
            Degrees(45.0),
            float(window->width()) / float(window->height()),
            1.0,
            1500.0
        );

        create_node<smlt::DirectionalLight>();
        cr_yield();
    }

private:
    CameraPtr camera_;
    PipelinePtr pipeline_;
};


class Q2Sample: public smlt::Application {
public:
    Q2Sample(const smlt::AppConfig& config):
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        scenes->activate("main");
        return true;
    }
};


int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Quake 2 Mesh Loader";
    config.fullscreen = false;

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
#endif

    Q2Sample app(config);
    return app.run();
}
