#ifndef TEXT_H
#define TEXT_H

#include "../../generic/managed.h"
#include "../../types.h"
#include "font.h"

#include <string>
#include <map>

namespace kglt {
namespace extra {
namespace ui {

class Text :
    public Managed<Text> {
public:
    Text(Font::ptr font, Vec2 position, const unicode& text);
    ~Text();

    float width_in_pixels() const;

    EntityID entity_id() const { return entity_; }
    void set_position(Vec2 position);
    void set_colour(const Colour& colour);
private:
    Interface& interface_;
    Font::ptr font_;

    MeshID mesh_;
    EntityID entity_;
    std::map<TextureID, MaterialID> materials_;
};

}
}
}

#endif // TEXT_H
