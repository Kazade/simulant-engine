#pragma once

#include <string>
#include <vector>

#include "../asset.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../generic/optional.h"
#include "../loadable.h"
#include "../types.h"

namespace smlt {

struct SpritesheetFrame {
    std::string name;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    /* Duration of this frame in milliseconds, as specified in the atlas
     * (0 if the atlas didn't specify one). */
    uint32_t duration_ms = 0;
};

struct SpritesheetAnimation {
    std::string name;
    uint32_t start_frame = 0;
    uint32_t end_frame = 0;
};

/* A collection of named, arbitrarily-placed frames on a single texture,
 * along with any named animations (frame ranges) defined for it. Loaded
 * from the TexturePacker-style "array" JSON atlas format written by the
 * sprite_gen tool. */
class Spritesheet:
    public Asset,
    public Loadable,
    public generic::Identifiable<AssetID>,
    public RefCounted<Spritesheet>,
    public ChainNameable<Spritesheet> {

public:
    Spritesheet(AssetID id, AssetManager* asset_manager);

    const char* asset_type_name() const override {
        return "Spritesheet";
    }

    uint64_t estimated_size_in_bytes() const override {
        /* texture_ isn't counted - it references a Texture that's already
         * logged (and sized) independently. */
        return sizeof(*this) + (frames_.size() * sizeof(SpritesheetFrame)) +
               (animations_.size() * sizeof(SpritesheetAnimation));
    }

    void set_texture(TexturePtr texture);
    TexturePtr texture() const;

    void push_frame(const SpritesheetFrame& frame);
    std::size_t frame_count() const;
    const SpritesheetFrame* frame(std::size_t i) const;
    optional<std::size_t> find_frame(const std::string& name) const;

    void push_animation(const SpritesheetAnimation& animation);
    std::size_t animation_count() const;
    const SpritesheetAnimation* animation(std::size_t i) const;

private:
    TexturePtr texture_;
    std::vector<SpritesheetFrame> frames_;
    std::vector<SpritesheetAnimation> animations_;
};

} // namespace smlt
