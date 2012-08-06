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

ui::LabelID UI::new_label() {
    if(!default_font_id_) {
        throw std::logic_error("You must give the UI a default font before creating ui elements");
    }

    static ui::LabelID counter = 0;
    ui::LabelID id = 0;
    {
        boost::mutex::scoped_lock lock(ui_lock_);
        id = ++counter;
        labels_.insert(std::make_pair(id, ui::Label::ptr(new ui::Label(&this->scene()))));
    }

    ui::Label& new_label = label(id);
    new_label.set_parent(this);
    new_label._initialize(scene());


    return id;
}

ui::Label& UI::label(ui::LabelID label_id) {
    boost::mutex::scoped_lock lock(ui_lock_);

    if(!container::contains(labels_, label_id)) {
        throw DoesNotExist<ui::LabelID>();
    }

    return *labels_[label_id];
}

}
