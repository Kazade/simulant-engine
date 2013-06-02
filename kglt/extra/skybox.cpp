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

SkyBox::SkyBox(kglt::Stage& stage, kglt::TextureID texture, float size, CameraID cam):
    stage_(stage),
    camera_id_(cam) {

    actor_ = &stage.actor(stage.new_actor(stage.new_mesh()));

    kglt::MeshPtr mesh = actor_->mesh().lock();
    kglt::procedural::mesh::cube(mesh, size);

    kglt::MaterialPtr mat = stage.material(stage.new_material()).lock();
    stage.window().loader_for("kglt/materials/generic_multitexture.kglm")->into(*mat);

    mat->technique().pass(0).set_texture_unit(0, texture);
    mat->technique().pass(0).set_depth_test_enabled(false);
    mat->technique().pass(0).set_depth_write_enabled(false);

    mesh->set_material_id(mat->id());
    mesh->reverse_winding();

    actor_->set_render_priority(RENDER_PRIORITY_BACKGROUND);
    actor_->attach_to_camera(cam);

    //Skyboxes shouldn't rotate based on their parent (e.g. the camera)
    actor_->lock_rotation(0, 0, 1, 0);
}

StarField::StarField(Stage& stage, CameraID cam) {
    //Generate a starfield texture
    texture_id_ = stage.new_texture();
    kglt::procedural::texture::starfield(stage.texture(texture_id_).lock());
    skybox_.reset(new SkyBox(stage, texture_id_, 500.0f, cam));
}


}
}
