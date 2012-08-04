#ifndef UI_H
#define UI_H

#include <map>

#include <boost/thread/thread.hpp>
#include <kazmath/mat4.h>

#include "object.h"
#include "generic/visitor.h"
#include "kaztext/kaztext.h"
#include "ui/types.h"

namespace kglt {

namespace ui {
    class Label;
}

class UI :
    public Object {

public:
    VIS_DEFINE_VISITABLE();

    UI():
        default_font_id_(0) {
        move_to(0.0, 0.0, -1.0);
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

private:
    std::map<ui::LabelID, std::tr1::shared_ptr<ui::Label> > labels_;
    kglt::FontID default_font_id_;

    boost::mutex ui_lock_;

    kmMat4 tmp_projection_;
};

}

#endif // UI_H
