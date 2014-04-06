#include "kglt/kglt.h"
#include "kglt/shortcuts.h"

class Q2Sample: public kglt::App {

public:
    Q2Sample():
        App("Quake 2 Renderer") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {
        window().loader_for("sample_data/sample.bsp")->into(scene());
        stage()->set_ambient_light(kglt::Colour(0.02, 0.02, 0.02, 1.0));
        return true;
    }

    void do_step(double dt) {}
    void do_cleanup() {}
};


int main(int argc, char* argv[]) {
    Q2Sample app;
    return app.run();
}
