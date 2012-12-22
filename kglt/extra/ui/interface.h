#ifndef INTERFACE_H
#define INTERFACE_H

#include "../../generic/managed.h"
#include "../../generic/manager.h"

#include "./types.h"
#include"../../types.h"

#include "widgets/label.h"

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
    void load_font(const std::string& ttf_file, uint8_t font_height);

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

private:
    Scene& scene_;

    uint32_t width_;
    uint32_t height_;
};

}
}
}

#endif // INTERFACE_H
