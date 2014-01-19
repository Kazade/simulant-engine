#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/extra.h"

using namespace kglt::extra;

class Sample2D: public kglt::App {
public:
    Sample2D():
        App("KGLT Sprite Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {

        //Automatically calculate an orthographic projection, taking into account the aspect ratio
        //and the passed height. For example, passing a height of 2.0 would mean the view would extend
        //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
        //window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
        scene().camera().set_orthographic_projection_from_height(16, float(window().width()) / float(window().height()));

        return true;
    }

    void do_step(double dt) {
        static bool shown = false;
        if(initialized() && !shown) {
            shown = true;
            //Load a sprite grid, from the 'Testing Name 1' layer in a tmx file
            sprite_grid_ = SpriteGrid::new_from_file(scene(), kglt::StageID(), "sample_data/tiled/example.tmx", "Layer 1");
            window().message_bar().inform("Sample demonstrating 2D sprites");
        }
    }
    void do_cleanup() {
        sprite_grid_.reset();
    }

    SpriteGrid::ptr sprite_grid_;
};


int main(int argc, char* argv[]) {
    Sample2D app;
    return app.run();
}
