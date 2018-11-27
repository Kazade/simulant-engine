/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "../types.h"

#include "../generic/managed.h"
#include "../generic/identifiable.h"
#include "../generic/property.h"
#include "../interfaces/printable.h"
#include "../interfaces/nameable.h"
#include "../interfaces/updateable.h"
#include "../nodes/sprite.h"

namespace smlt {

class BackgroundManager;

enum BackgroundType {
    BACKGROUND_TYPE_SCROLL,
    BACKGROUND_TYPE_ANIMATED
};

enum BackgroundResizeStyle {
    BACKGROUND_RESIZE_ZOOM,
    BACKGROUND_RESIZE_SCALE
};

class Background:
    public Managed<Background>,
    public generic::Identifiable<BackgroundID>,
    public Updateable,
    public Nameable,
    public Printable {

public:
    Background(BackgroundID background_id, BackgroundManager *manager, BackgroundType type);

    bool init() override;
    void cleanup() override;
    void update(float dt) override;

    void set_horizontal_scroll_rate(float x_rate);
    void set_vertical_scroll_rate(float y_rate);
    void set_texture(TextureID texture_id);
    void set_spritesheet(TextureID texture_id, float frame_width, float frame_height, SpritesheetAttrs attrs=SpritesheetAttrs());
    void set_resize_style(BackgroundResizeStyle style);

    //Ownable interface
    void ask_owner_for_destruction();

    Property<Background, Sprite> sprite = {this, &Background::sprite_};
    Property<Background, Stage> stage = {this, &Background::stage_};

    unicode to_unicode() const override {
        return Nameable::to_unicode();
    }

private:
    BackgroundManager* manager_;

    BackgroundType type_;
    StagePtr stage_;
    CameraPtr camera_;
    PipelinePtr pipeline_;
    SpritePtr sprite_;

    BackgroundResizeStyle style_ = BACKGROUND_RESIZE_ZOOM;
    float x_rate_ = 0.0;
    float y_rate_ = 0.0;
};

}

#endif // BACKGROUND_H
