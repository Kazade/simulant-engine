#include "spritesheet.h"

namespace smlt {

Spritesheet::Spritesheet(AssetID id, AssetManager* asset_manager):
    Asset(asset_manager),
    generic::Identifiable<AssetID>(id) {
}

void Spritesheet::set_texture(TexturePtr texture) {
    texture_ = texture;
}

TexturePtr Spritesheet::texture() const {
    return texture_;
}

void Spritesheet::push_frame(const SpritesheetFrame& frame) {
    frames_.push_back(frame);
}

std::size_t Spritesheet::frame_count() const {
    return frames_.size();
}

const SpritesheetFrame* Spritesheet::frame(std::size_t i) const {
    if(i >= frames_.size()) {
        return nullptr;
    }

    return &frames_[i];
}

optional<std::size_t> Spritesheet::find_frame(const std::string& name) const {
    for(std::size_t i = 0; i < frames_.size(); ++i) {
        if(frames_[i].name == name) {
            return i;
        }
    }

    return no_value;
}

void Spritesheet::push_animation(const SpritesheetAnimation& animation) {
    animations_.push_back(animation);
}

std::size_t Spritesheet::animation_count() const {
    return animations_.size();
}

const SpritesheetAnimation* Spritesheet::animation(std::size_t i) const {
    if(i >= animations_.size()) {
        return nullptr;
    }

    return &animations_[i];
}

} // namespace smlt
