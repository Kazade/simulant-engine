//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "skybox.h"

#include "../camera.h"
#include "../mesh.h"
#include "../stage.h"
#include "../material.h"
#include "../procedural/mesh.h"
#include "../procedural/texture.h"
#include "../window_base.h"
#include "../actor.h"
#include "../loader.h"

namespace smlt {
namespace extra {

SkyBox::SkyBox(StagePtr stage, smlt::TextureID texture, CameraID cam, float size):
    stage_(stage),
    camera_id_(cam) {

    actor_ = stage->new_actor_with_mesh(stage->assets->new_mesh_as_cube(size));

    auto mat = stage->assets->material(stage->assets->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY));

    mat->pass(0)->set_texture_unit(0, texture);
    mat->pass(0)->set_depth_test_enabled(false);
    mat->pass(0)->set_depth_write_enabled(false);

    {
        auto actor = stage->actor(actor_);
        actor->mesh()->set_material_id(mat->id());
        actor->mesh()->reverse_winding();

        actor->set_render_priority(RENDER_PRIORITY_BACKGROUND);
        actor->set_parent(camera_id_);

        //Skyboxes shouldn't rotate based on their parent (e.g. the camera)
        actor->set_absolute_rotation(Degrees(0), 0, 1, 0);
        actor->lock_rotation();
    }
}

StarField::StarField(StagePtr stage, CameraID cam) {
    //Generate a starfield texture
    texture_id_ = stage->assets->new_texture();
    auto tex = stage->assets->texture(texture_id_);
    smlt::procedural::texture::starfield(tex);
    skybox_.reset(new SkyBox(stage, texture_id_, cam));
}


}
}
