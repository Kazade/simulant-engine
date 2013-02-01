#ifndef INTERFACE_H
#define INTERFACE_H

#include "../../kazbase/unicode.h"

#include "../../generic/managed.h"
#include "../../generic/manager.h"

#include "./types.h"
#include"../../types.h"

#include "widgets/label.h"
#include "font.h"

namespace kglt {
namespace extra {
namespace ui {

class Interface;

typedef generic::TemplatedManager<Interface, Label, LabelID> LabelManager;

class Interface :
    public Managed<Interface>,
    public LabelManager {

public:
    Interface(Scene& scene, uint32_t width_in_pixels, uint32_t height_in_pixels);
    unicode load_font(const std::string& ttf_file, uint8_t font_height);

    LabelID new_label();
    Label& label(LabelID l);
    void delete_label(LabelID l);

    ContainerID new_container();
    Container& container(ContainerID c);
    void delete_container(ContainerID c);

    TextInputID new_text_input();
    void text_input(TextInputID entry);
    void delete_text_input(TextInputID entry);

    ButtonID new_button();
    void button(ButtonID button);
    void delete_button();

    Scene& scene() { return scene_; }
    SubScene& subscene();

    unicode default_font() const { return default_font_; }
    uint16_t default_size() const { return default_size_; }

    uint16_t width_in_pixels() const { return width_; }
    uint16_t height_in_pixels() const { return height_; }

    Font::ptr _font(const unicode& name, uint16_t size) const { return fonts_.at(name).at(size); }

private:    
    Scene& scene_;
    SubSceneID subscene_;

    CameraID camera_;

    uint32_t width_;
    uint32_t height_;

    std::map<unicode, std::map<uint16_t, Font::ptr> > fonts_;
    unicode default_font_;
    uint16_t default_size_;

    friend class Widget;
};

}
}
}

#endif // INTERFACE_H
