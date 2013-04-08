#include "kglt/kglt.h"
#include "kglt/shortcuts.h"
#include "kglt/additional.h"
#include "kglt/extra/ui/interface.h"

using kglt::extra::Sprite;
using kglt::extra::SpriteStripLoader;
using kglt::extra::Background;
using namespace kglt::extra;

int main(int argc, char* argv[]) {
    //Set up logging to stdio
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

    kglt::Window::ptr window = kglt::Window::create();
    window->set_title("KGLT Parallax Sample");

    kglt::Scene& scene = window->scene();
    kglt::SubScene& subscene = scene.subscene();

    //Automatically calculate an orthographic projection, taking into account the aspect ratio
    //and the passed height. For example, passing a height of 2.0 would mean the view would extend
    //+1 and -1 in the vertical direction, -1.0 - +1.0 near/far, and width would be calculated from the aspect
    //window.scene().pass().viewport().configure(kglt::VIEWPORT_TYPE_BLACKBAR_16_BY_9);
    subscene.camera().set_orthographic_projection_from_height((float) 224 / (float) 40, 16.0 / 9.0);

    //Load the strip of sprites into separate textures
    SpriteStripLoader loader(subscene, "sample_data/sonic.png", 64);
    std::vector<kglt::TextureID> frames = loader.load_frames();

    //Construct a Sprite object that takes care of handling materials, meshes etc.
    Sprite::ptr sprite = Sprite::create(scene, subscene.id());
    sprite->add_animation("running", container::slice(frames, 31, 35), 0.5);
    sprite->set_render_dimensions(1.5, 1.5);
    sprite->move_to(0.0, -2.0, -1.0);
/*
    ui::Interface::ptr interface = ui::Interface::create(scene, 800, 600);  
    interface->load_font("sample_data/sample.ttf", 12);

    ui::LabelID l = interface->new_label();
    interface->label(l).set_text("Score: 12345");
    interface->label(l).set_position(ui::Ratio(0.1), ui::Ratio(0.8));
    interface->label(l).set_size(ui::Ratio(0.4), ui::Ratio(0.1));
    interface->label(l).set_foreground_colour(kglt::Colour::red);
*/
    Background::ptr background = Background::create(scene);

    //Alternatively window.scene().background().add_layer("sample_data/parallax/back_layer.png", BACKGROUND_FILL);
    background->add_layer("sample_data/parallax/back_layer.png");
    background->add_layer("sample_data/parallax/middle_layer.png");
    background->add_layer("sample_data/parallax/front_layer.png");

    while(window->update()) {
        background->layer(0).scroll_x(0.1 * window->delta_time());
        background->layer(1).scroll_x(0.2 * window->delta_time());
        background->layer(2).scroll_x(1.0 * window->delta_time());
    }

    return 0;
}

