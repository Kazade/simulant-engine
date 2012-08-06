#ifndef TEXT_H
#define TEXT_H

#include <tr1/memory>
#include <string>
#include "types.h"

#include "object.h"
#include "generic/visitor.h"
#include "font.h"

namespace kglt {

class Text :
    public Object {

public:
    VIS_DEFINE_VISITABLE();

    typedef std::tr1::shared_ptr<Text> ptr;

    Text(Scene* scene):
        Object(scene) {}

    void apply_font(FontID font_id);
    Font& font();

    void set_text(const std::string& utf8_text);
    std::string text() const { return text_; }
    uint32_t length() const; //Returns the actual length of the utf-8 string
    double width_in_pixels();

    void set_colour(const kglt::Colour& colour) { colour_ = colour; }
    kglt::Colour& colour() { return colour_; }

private:
    FontID applied_font_;
    std::string text_;

    kglt::Colour colour_;
};

}

#endif // TEXT_H
