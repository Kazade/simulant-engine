#include "material_property_type.h"
#include "../../texture.h"

namespace smlt {

TextureUnit::TextureUnit(const TexturePtr& texture):
    texture_(texture),
    texture_id_(texture_->id()) {}

void TextureUnit::scroll_x(float amount) {
    texture_matrix_[12] += amount;
}

void TextureUnit::scroll_y(float amount) {
    texture_matrix_[13] += amount;
}

const TextureID& TextureUnit::texture_id() const {
    return texture_id_;
}

}
