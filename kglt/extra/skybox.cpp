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

namespace kglt {
namespace extra {

SkyBox::SkyBox(StagePtr stage, kglt::TextureID texture, CameraID cam, float size):
    stage_(stage),
    camera_id_(cam) {

    actor_ = stage->new_actor_with_mesh(stage->new_mesh_as_cube(size));

    auto mat = stage->material(stage->new_material_from_file("kglt/materials/generic_multitexture.kglm"));

    mat->pass(0).set_texture_unit(0, texture);
    mat->pass(0).set_depth_test_enabled(false);
    mat->pass(0).set_depth_write_enabled(false);

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
    texture_id_ = stage->new_texture();
    auto tex = stage->texture(texture_id_);
    kglt::procedural::texture::starfield(tex.__object);
    skybox_.reset(new SkyBox(stage, texture_id_, cam));
}


}
}
