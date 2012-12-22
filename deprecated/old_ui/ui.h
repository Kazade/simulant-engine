#ifndef UI_H
#define UI_H

#include <map>

#include <boost/thread/thread.hpp>
#include <kazmath/mat4.h>

#include "object.h"
#include "generic/visitor.h"
#include "generic/manager.h"
#include "kaztext/kaztext.h"
#include "ui/types.h"
#include "ui/label.h"
#include "scene.h"

namespace kglt {

class UI :
    public generic::TemplatedManager<UI, ui::Label, ui::LabelID> {

public:
    UI(Scene* scene);

    template<typename Element, typename ElementID>
    void post_create_callback(Element& element, ElementID id) {
        element._initialize(scene_);
        element.set_font(default_font_id_);
    }

    /*void pre_visit(ObjectVisitor& visitor);
    void post_visit(ObjectVisitor &visitor);*/

    ui::LabelID new_label();
    ui::Label& label(ui::LabelID label_id);

    void set_default_font_id(kglt::FontID fid) {
        default_font_id_ = fid;
    }

    kglt::FontID default_font_id() const {
        return default_font_id_;
    }

    Scene& scene() { return scene_; }
private:
    Scene& scene_;

    kglt::FontID default_font_id_;

    boost::mutex ui_lock_;

    kmMat4 tmp_projection_;
};

}

#endif // UI_H
