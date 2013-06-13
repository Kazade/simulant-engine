#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"

using namespace kglt::extra;

class Sample2D: public kglt::App {
public:
    Sample2D():
        App("KGLT Sprite Sample") {

        window().set_logging_level(kglt::LOG_LEVEL_DEBUG);
    }

private:
    bool do_init() {
        //Construct a Sprite object that takes care of handling materials, meshes etc.
        sprite_ = Sprite::create(
            scene().stage_ref(stage().id()),
            "sample_data/sonic.png",
            FrameSize(64, 64)
        );

        sprite_->add_animation("stand", FrameRange(4, 6), 0.5);
        sprite_->set_render_dimensions(1.5, 1.5);
        sprite_->move_to(0.0, 0.0, -1.0);

        //Automatically calculate an orthographic projection, taking into account the aspect ratio
        //and the passed height. For example, passing a height of 2.0 would mean the view would extend
        //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
        //window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
        scene().camera().set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);

        return true;
    }

    void do_step(float dt) {}
    void do_cleanup() {}

    Sprite::ptr sprite_;
};


int main(int argc, char* argv[]) {
    Sample2D app;
    return app.run();
}
