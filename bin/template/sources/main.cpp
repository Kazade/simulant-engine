#include <simulant/simulant.h>

#include "scenes/game.h"

using namespace smlt;

class __project_name_pascal__:
    public Application {

public:
    __project_name_pascal__(const AppConfig& config):
        Application(config) {}

    bool init() {
        // Register screens here
        scenes->register_scene<GameScene>("main");

        return true;
    }
};

extern "C" {

#ifdef __ANDROID__

#include <SDL_main.h>
#include <gpg/gpg.h>

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    gpg::AndroidInitialization::JNI_OnLoad(vm);
    return JNI_VERSION_1_6;
}

    int SDL_main(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    AppConfig config;
    config.title = "__project_name__";
    config.search_paths.push_back("assets");

    // Dreamcast only
    config.search_paths.push_back("/cd/assets");

#ifndef NDEBUG
    config.log_level = smlt::LOG_LEVEL_DEBUG;
#endif

    __project_name_pascal__ app(config);
    return app.run();
}

}
