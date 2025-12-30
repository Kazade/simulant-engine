#include "simulant/simulant.h"

#include <cstdlib>
#include <ctime>

using namespace smlt;

class GameScene : public smlt::Scene {
public:
    GameScene(smlt::Window* window):
        smlt::Scene(window) {}

    void on_load() override {
        auto prefab = assets->load_prefab("assets/samples/character-a.glb");
        prefab_ = create_child<smlt::PrefabInstance>(prefab);

        auto anim_controller = prefab_->find_mixin<AnimationController>();
        auto animations = anim_controller->animation_names();
        if(anim_controller && !animations.empty()) {
            anim_controller->play(animations[0]);
            anim_controller->queue(animations[1]);
            anim_controller->queue(animations[2]);
            anim_controller->queue(animations[3]);
            anim_controller->queue(animations[4]);
            anim_controller->queue(animations[5]);
        }

        prefab_->transform->set_position(smlt::Vec3(0, -1, -5.0f));

        auto camera = create_child<smlt::Camera3D>({
            {"znear",  0.1f                  },
            {"zfar",   100.0f                },
            {"aspect", window->aspect_ratio()},
            {"yfov",   45.0f                 }
        });

        camera->set_perspective_projection(Degrees(45.0),
                                           window->aspect_ratio(), 1.0, 1000.0);

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
    config.title = "Animation Sample";

#ifdef __DREAMCAST__
    config.width = 640;
    config.height = 480;
#else
    config.width = 1280;
    config.height = 960;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;
#endif

    config.development.force_renderer = "gl1x";

    AnimDemo app(config);
    return app.run();
}
