#ifndef UI_H
#define UI_H

#include <map>

#include <boost/thread/thread.hpp>

#include "object.h"
#include "object_visitor.h"

#include "kaztext/kaztext.h"
#include "ui/types.h"

namespace kglt {

namespace ui {
    class Label;
}

class UI : public Object {
public:
    UI():
        default_font_id_(0) {
        move_to(0.0, 0.0, -1.0);
    }

    void accept(ObjectVisitor& visitor) {
        visitor.pre_visit(this);

        for(Object* child: children_) {
            child->accept(visitor);
        }

        if(is_visible()) {
            visitor.visit(this);
        }
        visitor.post_visit(this);
    }

    void pre_visit(ObjectVisitor& visitor);
    void post_visit(ObjectVisitor &visitor);

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
};

}

#endif // UI_H
