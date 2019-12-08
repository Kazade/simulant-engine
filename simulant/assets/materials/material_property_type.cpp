#include "material_property_type.h"
#include "../../texture.h"

namespace smlt {

TextureUnit::TextureUnit(const TexturePtr& texture):
    texture_(texture),
    texture_id_(texture_->id()) {}

void TextureUnit::scroll_x(float amount) {
    Mat4 diff = Mat4::as_translation(Vec3(amount, 0, 0));
    *texture_matrix_ = *texture_matrix_ * diff;
}

void TextureUnit::scroll_y(float amount) {
    Mat4 diff = Mat4::as_translation(Vec3(0, amount, 0));
    *texture_matrix_ = *texture_matrix_ * diff;
}

const TextureID& TextureUnit::texture_id() const {
    return texture_id_;
}

}
