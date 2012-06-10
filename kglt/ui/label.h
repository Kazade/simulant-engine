#ifndef LABEL_H
#define LABEL_H

#include <tr1/memory>
#include "element.h"

namespace kglt {
namespace ui {

class Label : public Element {
public:
    typedef std::tr1::shared_ptr<Label> ptr;

    void set_text(const std::string& text);
    void set_position(float x, float y);
    void set_font_size(float height);
};

}
}
#endif // LABEL_H
