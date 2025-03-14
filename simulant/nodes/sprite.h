/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPRITE_H
#define SPRITE_H

#include "stage_node.h"

#include "../animation.h"
#include "../generic/identifiable.h"
#include "../generic/managed.h"
#include "../interfaces.h"
#include "../macros.h"
#include "../sound.h"
#include "../types.h"

namespace smlt {

class KeyFrameAnimationState;
class SpriteManager;

struct SpritesheetAttrs {
    uint32_t margin = 0;
    uint32_t spacing = 0;
    uint32_t padding_vertical = 0;
    uint32_t padding_horizontal = 0;
};

class Sprite:
    public ContainerNode,
    public KeyFrameAnimated,
    public ChainNameable<Sprite> {

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_SPRITE, "sprite");

    bool on_create(Params params) override;
    bool on_destroy() override;
    void on_update(float dt) override;

    Sprite(Scene* owner);

    void set_render_dimensions(float width, float height);
    void set_render_dimensions_from_width(float width);
    void set_render_dimensions_from_height(float height);

    void set_render_priority(smlt::RenderPriority priority);

    void set_alpha(float alpha);

    float alpha() const {
        return alpha_;
    }
    MaterialPtr material() const {
        return material_;
    }

    void set_spritesheet(TexturePtr texture, uint32_t frame_width,
                         uint32_t frame_height,
                         SpritesheetAttrs attrs = SpritesheetAttrs());

    void flip_vertically(bool value = true);
    void flip_horizontally(bool value = true);

    const AABB& aabb() const override;

private:
    float frame_width_ = 0;
    float frame_height_ = 0;
    float sprite_sheet_margin_ = 0;
    float sprite_sheet_spacing_ = 0;
    std::pair<uint32_t, uint32_t> sprite_sheet_padding_;
    float render_width_ = 1.0;
    float render_height_ = 1.0;

    ActorPtr actor_ = nullptr;
    MeshPtr mesh_;
    MaterialPtr material_;

    float image_width_ = 0;
    float image_height_ = 0;

    float alpha_ = 1.0f;

    void update_texture_coordinates();

    bool flipped_vertically_ = false;
    bool flipped_horizontally_ = false;

    void refresh_animation_state(uint32_t current_frame, uint32_t next_frame,
                                 float interp) {
        _S_UNUSED(current_frame);
        _S_UNUSED(next_frame);
        _S_UNUSED(interp);

        update_texture_coordinates();
    }

    std::shared_ptr<KeyFrameAnimationState> animation_state_;

public:
    Property<ActorPtr Sprite::*> actor = {this, &Sprite::actor_};
    Property<decltype(&Sprite::animation_state_)> animations = {
        this, &Sprite::animation_state_};
};

} // namespace smlt
#endif // SPRITE_H
