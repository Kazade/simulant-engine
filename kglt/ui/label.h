#ifndef LABEL_H
#define LABEL_H

#include <tr1/memory>
#include "element.h"

namespace kglt {

class Text;
class UI;

namespace ui {

class Label : public Element {
public:
    typedef std::tr1::shared_ptr<Label> ptr;

    Label(UI* ui):
        Element(ui),
        text_id_(0) {}

    void set_text(const std::string& text);
    std::string text() const;
    void set_font(kglt::FontID fid);

    virtual double width();
    virtual double height();

    virtual void _initialize(Scene &scene);

private:
    TextID text_id_;
    Text& text_object();
    const Text& text_object() const;
};

}
}
#endif // LABEL_H
