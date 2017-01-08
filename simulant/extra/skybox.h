/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SKYBOX_H
#define SKYBOX_H

#include "../types.h"
#include "../generic/managed.h"
#include "../generic/auto_weakptr.h"

namespace smlt{

namespace extra {

class SkyBox :
    public Managed<SkyBox> {

public:
    SkyBox(StagePtr stage, TextureID texture, CameraID cam, float size=500.0f);
    SkyBox(StagePtr stage, TextureID front, TextureID back, TextureID left, TextureID right, TextureID top, TextureID bottom);

private:
    StagePtr stage_;

    MaterialID material_id_;
    ActorID actor_;

    CameraID camera_id_;
};

class StarField :
    public Managed<StarField> {

public:
    StarField(StagePtr stage, CameraID cam);

private:
    SkyBox::ptr skybox_;
    TextureID texture_id_;
};

}
}

#endif // SKYBOX_H
