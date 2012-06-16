#ifndef TEXT_H
#define TEXT_H

#include <tr1/memory>
#include <string>
#include "types.h"

#include "object.h"
#include "object_visitor.h"

namespace kglt {

class Text : public Object {
public:
    typedef std::tr1::shared_ptr<Text> ptr;

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

    void apply_font(FontID font_id);
    Font& font();

    void set_text(const std::string& utf8_text);
    std::string text() const { return text_; }
    uint32_t length() const; //Returns the actual length of the utf-8 string
    double width_in_pixels();

private:
    FontID applied_font_;
    std::string text_;
};

}

#endif // TEXT_H
