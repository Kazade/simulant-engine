#ifndef LABEL_H
#define LABEL_H

#include <tr1/memory>

#include "../generic/identifiable.h"
#include "element.h"
#include "types.h"

namespace kglt {

class Text;
class UI;

namespace ui {

class Label :
    public Element,
    public generic::Identifiable<LabelID> {

public:
    typedef std::tr1::shared_ptr<Label> ptr;

    Label(UI* ui, LabelID id):
        Element(ui),
        generic::Identifiable<LabelID>(id),
        text_id_(0) {}

    void set_text(const std::string& text);
    std::string text() const;
    void set_font(kglt::FontID fid);

    virtual double width();
    virtual double height();

    virtual void _initialize(Scene &scene);

    virtual void set_foreground_colour(const kglt::Colour& colour);
private:
    TextID text_id_;
    Text& text_object();
    const Text& text_object() const;
};

}
}
#endif // LABEL_H
