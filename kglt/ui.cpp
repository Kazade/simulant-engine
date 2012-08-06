#include "ui.h"
#include "renderer.h"
#include "scene.h"
#include "window_base.h"

#include "../kazbase/os/path.h"
#include "../kazbase/exceptions.h"

#include "ui/label.h"

namespace kglt {

/*
void UI::pre_visit(ObjectVisitor& visitor) {
    if(Renderer* renderer = dynamic_cast<Renderer*>(&visitor)) {
        /**
         * FIXME:
         * See background.cpp for why this is all bollocks
         *
        tmp_projection_ = renderer->projection_matrix();

        kmMat4 new_proj;
        kmMat4OrthographicProjection(&new_proj, 0, scene().window().width(), 0, scene().window().height(), -1.0, 10.0);
        renderer->set_projection_matrix(new_proj);
    }
}

void UI::post_visit(ObjectVisitor &visitor) {
    if(Renderer* renderer = dynamic_cast<Renderer*>(&visitor)) {
        renderer->set_projection_matrix(tmp_projection_);
    }
}*/

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
