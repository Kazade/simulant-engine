#include <SDL_main.h>
#include <kglt/kglt.h>
#include <kazbase/random.h>

class {PROJECT_NAME_TITLE}: public kglt::Application {
public:
    {PROJECT_NAME_TITLE}():
        kglt::Application("{PROJECT_TITLE}", 0, 0, 0, true) {}

    bool do_init() {
        window().set_logging_level(kglt::LOG_LEVEL_INFO);
        window().resource_locator().add_search_path("./assets");

        // Register screens here
        return true;
    }
};

extern "C" {

#ifdef __ANDROID__

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
    seed(); //Make sure things happen randomly
    {PROJECT_NAME_TITLE} app;
    return app.run();
}

}
