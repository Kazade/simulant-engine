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
#include "../procedural/geom_factory.h"

namespace kglt {
namespace extra {

SkyBox::SkyBox(kglt::Stage& stage, kglt::TextureID texture, float size, CameraID cam):
    stage_(stage),
    camera_id_(cam) {

    actor_ = &stage.actor(stage.geom_factory().new_cube(size));

    auto mat = stage.material(stage.new_material_from_file("kglt/materials/generic_multitexture.kglm"));

    mat->technique().pass(0).set_texture_unit(0, texture);
    mat->technique().pass(0).set_depth_test_enabled(false);
    mat->technique().pass(0).set_depth_write_enabled(false);

    actor_->mesh().lock()->set_material_id(mat->id());
    actor_->mesh().lock()->reverse_winding();

    actor_->set_render_priority(RENDER_PRIORITY_BACKGROUND);
    actor_->attach_to_camera(cam);

    //Skyboxes shouldn't rotate based on their parent (e.g. the camera)
    actor_->set_absolute_rotation(Degrees(0), 0, 1, 0);
    actor_->lock_rotation();
}

StarField::StarField(Stage& stage, CameraID cam) {
    //Generate a starfield texture
    texture_id_ = stage.new_texture();
    auto tex = stage.texture(texture_id_);
    kglt::procedural::texture::starfield(tex.__object);
    skybox_.reset(new SkyBox(stage, texture_id_, 500.0f, cam));
}


}
}
