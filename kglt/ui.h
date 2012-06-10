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

    void register_font(const std::string& name, const std::string& ttf_path);

    ui::LabelID new_label();
    ui::Label& label(ui::LabelID label_id);

private:
    std::map<std::string, std::string> font_paths_;
    std::map<ui::LabelID, std::tr1::shared_ptr<ui::Label> > labels_;

    boost::mutex ui_lock_;
};

}

#endif // UI_H
