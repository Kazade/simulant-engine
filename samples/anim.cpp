#include "simulant/simulant.h"

#include <cstdlib>
#include <ctime>

using namespace smlt;

class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window):
        smlt::Scene(window) {}

    void on_load() override {
        auto prefab = assets->load_prefab("assets/samples/BoxTextured.gltf");
        prefab_ = create_child<smlt::PrefabInstance>(prefab);

        prefab_->transform->set_position(smlt::Vec3(0, 0, -10));
        for(auto& child: prefab_->each_descendent()) {
            smlt::ActorPtr actor = dynamic_cast<smlt::ActorPtr>(&child);
            if(actor) {
                auto aabb = actor->aabb();
                auto mesh = actor->base_mesh();

                fprintf(stderr, "%s\n", actor->name().c_str());
            }
        }

        auto camera = create_child<smlt::Camera3D>({
            {"znear",  0.1f                  },
            {"zfar",   100.0f                },
            {"aspect", window->aspect_ratio()},
            {"yfov",   60.0f                 }
        });

        camera->set_perspective_projection(
            Degrees(45.0), float(window->width()) / float(window->height()),
            1.0, 1000.0);

        camera->transform->set_position(Vec3(0, 10, 50));
        // camera->set_perspective_projection(
        //     smlt::Deg(60.0f), window->aspect_ratio(), 1.0f, 10000.0f);
        // camera->transform->look_at(prefab_->transform->position());

        auto layer = compositor->create_layer(prefab_, camera);
        layer->set_clear_flags(smlt::BUFFER_CLEAR_ALL);
        layer->viewport->set_color(smlt::Color::gray());
    }

private:
    std::vector<StageNodePtr> boxes_;

    smlt::PrefabInstance* prefab_ = nullptr;
};

class AnimDemo: public smlt::Application {
public:
    AnimDemo(const smlt::AppConfig& config) :
        smlt::Application(config) {}

private:
    bool init() {
        scenes->register_scene<GameScene>("main");
        scenes->activate("_loading"); // Show the loading screen in the meantime
        scenes->preload_in_background("main").then([this]() {
            scenes->activate("main");
        }); //Do loading in a background thread, but show immediately when done
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    smlt::AppConfig config;
    config.title = "Physics Sample";

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;
#endif

    AnimDemo app(config);
    return app.run();
}
