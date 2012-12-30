#include "../../scene.h"
#include "interface.h"
#include "font.h"
#include "kazbase/string.h"

namespace kglt {
namespace extra {
namespace ui {

SubScene& Interface::subscene() { return scene_.subscene(subscene_); }

Interface::Interface(Scene &scene, uint32_t width_in_pixels, uint32_t height_in_pixels):
    scene_(scene),
    width_(width_in_pixels),
    height_(height_in_pixels),
    default_size_(10) {

    subscene_ = scene.new_subscene(PARTITIONER_NULL); //Don't cull the UI
    camera_ = subscene().new_camera();

    subscene().camera(camera_).set_orthographic_projection(0, width_, 0, height_, -1.0, 1.0);
    scene_.pipeline().add_stage(subscene_, camera_);
}

unicode Interface::load_font(const std::string &ttf_file, uint8_t font_height) {
    Font::ptr font = Font::create(*this, ttf_file, font_height);

    //Special case the "Regular" style, because it's annoying to have to type Regular
    //when you just want the default font

    unicode name = unicode(" ").join({
        font->family_name(),
        (font->style_name() == unicode("Regular")) ? unicode("") : font->style_name()
    });

    fonts_[name][font_height] = font;

    if(default_font_.empty()) {
        default_font_ = name;
        default_size_ = font_height;
    }

    return name;
}

LabelID Interface::new_label() {
    return LabelManager::manager_new();
}

Label& Interface::label(LabelID l) {
    return LabelManager::manager_get(l);
}

void Interface::delete_label(LabelID l) {
    LabelManager::manager_delete(l);
}

}
}
}
