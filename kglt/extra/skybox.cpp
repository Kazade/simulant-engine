#include "skybox.h"

#include "../camera.h"
#include "../mesh.h"
#include "../scene.h"
#include "../material.h"
#include "../procedural/mesh.h"
#include "../procedural/texture.h"
#include "../window_base.h"
#include "../entity.h"

namespace kglt {
namespace extra {

SkyBox::SkyBox(kglt::SubScene& subscene, kglt::TextureID texture, float size, CameraID cam):
    subscene_(subscene),
    camera_id_(cam) {

    mesh_id_ = subscene.new_mesh();
    kglt::procedural::mesh::cube(subscene.mesh(mesh_id_), size);

    material_id_ = subscene.new_material();
    kglt::Material& mat = subscene.material(material_id_);
    subscene.scene().window().loader_for("kglt/materials/generic_multitexture.kglm")->into(mat);

    mat.technique().pass(0).set_texture_unit(0, texture);
    mat.technique().pass(0).set_depth_test_enabled(false);
    mat.technique().pass(0).set_depth_write_enabled(false);

    subscene.mesh(mesh_id_).set_material(mat.id());
    subscene.mesh(mesh_id_).reverse_winding();

    entity_id_ = subscene.new_entity(mesh_id_);
    subscene.entity(entity_id_).set_render_priority(-100000);    
    subscene.entity(entity_id_).set_parent(subscene.camera(cam));
}

StarField::StarField(SubScene& subscene, CameraID cam) {
    //Generate a starfield texture
    texture_id_ = subscene.new_texture();
    kglt::procedural::texture::starfield(subscene.texture(texture_id_));
    skybox_.reset(new SkyBox(subscene, texture_id_, 500.0f, cam));
}


}
}
