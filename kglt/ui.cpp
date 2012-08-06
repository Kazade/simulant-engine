#include "ui.h"
#include "renderer.h"
#include "scene.h"
#include "window_base.h"

#include "../kazbase/os/path.h"
#include "../kazbase/exceptions.h"

#include "ui/label.h"

namespace kglt {

UI::UI(Scene* scene):
    scene_(*scene),
    default_font_id_(0) {

    //Create the overlay for the UI
    overlay_ = scene_.new_overlay();

    Overlay& overlay = scene_.overlay(overlay_);
    overlay.set_ortho(0, scene_.window().width(), 0, scene_.window().height());

    generic::TemplatedManager<UI, ui::Label, ui::LabelID>::signal_post_create().connect(
        sigc::mem_fun(this, &UI::post_create_callback<ui::Label, ui::LabelID>)
    );

    //move_to(0.0, 0.0, -1.0);
}

ui::LabelID UI::new_label() {
    if(!default_font_id_) {
        throw std::logic_error("You must give the UI a default font before creating ui elements");
    }

    return generic::TemplatedManager<UI, ui::Label, ui::LabelID>::manager_new();
}

ui::Label& UI::label(ui::LabelID label_id) {
    return generic::TemplatedManager<UI, ui::Label, ui::LabelID>::manager_get(label_id);
}

}
