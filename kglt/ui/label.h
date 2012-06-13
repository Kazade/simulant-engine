#ifndef LABEL_H
#define LABEL_H

#include <tr1/memory>
#include "element.h"

namespace kglt {

class Text;

namespace ui {

class Label : public Element {
public:
    typedef std::tr1::shared_ptr<Label> ptr;

    void set_text(const std::string& text);

    virtual double width();
    virtual double height();

private:
    TextID text_id_;
    Text& text_object();

    void on_parent_set(Object *old_parent);
};

}
}
#endif // LABEL_H
