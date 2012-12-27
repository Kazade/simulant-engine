#include "../../scene.h"
#include "interface.h"
#include "font.h"
#include "kazbase/string.h"

namespace kglt {
namespace extra {
namespace ui {

Interface::Interface(Scene &scene, uint32_t width_in_pixels, uint32_t height_in_pixels):
    scene_(scene),
    width_(width_in_pixels),
    height_(height_in_pixels),
    default_size_(10) {

    scene_group_ = scene.new_scene_group();
    camera_ = scene.new_camera();

    scene.camera(camera_).set_orthographic_projection(0, width_, 0, height_, -1.0, 1.0);
    scene.pipeline().add_pass(scene_group_, TextureID(), camera_, ViewportID());
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
